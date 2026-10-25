// Audio processing unit: 2 square channels, wave channel, noise channel.
// Output is mixed to stereo and resampled to AUDIO_SAMPLE_RATE.
#include "gb.h"

#include <string.h>

#define NR(x) (gb->io[(x)])

static const uint8_t duty_table[4] = {0x01, 0x81, 0x87, 0x7e};
static const uint8_t read_mask[0x30] = {
	0x80, 0x3f, 0x00, 0xff, 0xbf, // NR10-14
	0xff, 0x3f, 0x00, 0xff, 0xbf, // NR20-24
	0x7f, 0xff, 0x9f, 0xff, 0xbf, // NR30-34
	0xff, 0xff, 0x00, 0x00, 0xbf, // NR40-44
	0x00, 0x00, 0x70,             // NR50-52
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // unused
};

// High-pass filter state to remove the DC offset of the unipolar mix.
static float hp_l, hp_r;

void apu_reset(GB *gb)
{
	memset(&gb->apu, 0, sizeof(gb->apu));
	hp_l = hp_r = 0;
}

uint8_t apu_read(GB *gb, uint8_t reg)
{
	if (reg >= 0x30)
		return NR(reg);
	if (reg == 0x26) {
		uint8_t v = gb->apu.power ? 0x80 : 0;
		for (int i = 0; i < 4; i++)
			if (gb->apu.ch[i].on)
				v |= (uint8_t)(1 << i);
		return v | 0x70;
	}
	return NR(reg) | read_mask[reg - 0x10];
}

static int square_period(APUChannel *ch)
{
	return (2048 - ch->freq) * 4;
}

static int sweep_calc(APUChannel *ch)
{
	int delta = ch->sweep_shadow >> ch->sweep_shift;
	int f = ch->sweep_down ? ch->sweep_shadow - delta : ch->sweep_shadow + delta;
	if (f > 2047)
		ch->on = false;
	return f;
}

static void trigger(GB *gb, int n)
{
	APUChannel *ch = &gb->apu.ch[n];
	ch->on = ch->dac;
	if (ch->length == 0)
		ch->length = n == 2 ? 256 : 64;
	if (n == 2) {
		ch->timer = (2048 - ch->freq) * 2;
		ch->wave_pos = 0;
		return;
	}
	uint8_t env = NR(0x12 + n * 5);
	ch->volume = env >> 4;
	ch->env_up = env & 0x08;
	ch->env_period = env & 7;
	ch->env_timer = ch->env_period ? ch->env_period : 8;
	if (n == 3) {
		ch->lfsr = 0x7fff;
		return;
	}
	ch->timer = square_period(ch);
	if (n == 0) {
		uint8_t s = NR(0x10);
		ch->sweep_period = (s >> 4) & 7;
		ch->sweep_down = s & 0x08;
		ch->sweep_shift = s & 7;
		ch->sweep_shadow = ch->freq;
		ch->sweep_timer = ch->sweep_period ? ch->sweep_period : 8;
		ch->sweep_enabled = ch->sweep_period || ch->sweep_shift;
		if (ch->sweep_shift)
			sweep_calc(ch);
	}
}

void apu_write(GB *gb, uint8_t reg, uint8_t val)
{
	if (reg >= 0x30) {
		NR(reg) = val;
		return;
	}
	if (reg == 0x26) {
		bool on = val & 0x80;
		if (!on && gb->apu.power) {
			for (int r = 0x10; r < 0x26; r++)
				NR(r) = 0;
			memset(gb->apu.ch, 0, sizeof(gb->apu.ch));
		}
		gb->apu.power = on;
		return;
	}
	if (!gb->apu.power)
		return;
	NR(reg) = val;

	int n = (reg - 0x10) / 5;
	int r = (reg - 0x10) % 5;
	if (n > 3)
		return; // NR50/NR51 are read directly when mixing
	APUChannel *ch = &gb->apu.ch[n];
	switch (r) {
	case 0:
		if (n == 2) {
			ch->dac = val & 0x80;
			if (!ch->dac)
				ch->on = false;
		}
		break;
	case 1:
		if (n == 2) {
			ch->length = 256 - val;
		} else {
			ch->length = 64 - (val & 0x3f);
			ch->duty = val >> 6;
		}
		break;
	case 2:
		if (n != 2) {
			ch->dac = (val & 0xf8) != 0;
			if (!ch->dac)
				ch->on = false;
		}
		break;
	case 3:
		ch->freq = (ch->freq & 0x700) | val;
		break;
	case 4:
		ch->freq = (ch->freq & 0xff) | ((val & 7) << 8);
		ch->length_enable = val & 0x40;
		if (val & 0x80)
			trigger(gb, n);
		break;
	}
}

static void frame_sequencer(GB *gb)
{
	int step = gb->apu.frame_seq_step;
	gb->apu.frame_seq_step = (step + 1) & 7;

	if (!(step & 1)) {
		for (int i = 0; i < 4; i++) {
			APUChannel *ch = &gb->apu.ch[i];
			if (ch->length_enable && ch->length > 0 && --ch->length == 0)
				ch->on = false;
		}
	}
	if (step == 2 || step == 6) {
		APUChannel *ch = &gb->apu.ch[0];
		if (--ch->sweep_timer <= 0) {
			ch->sweep_timer = ch->sweep_period ? ch->sweep_period : 8;
			if (ch->sweep_enabled && ch->sweep_period) {
				int f = sweep_calc(ch);
				if (f <= 2047 && ch->sweep_shift) {
					ch->freq = f;
					ch->sweep_shadow = f;
					sweep_calc(ch);
				}
			}
		}
	}
	if (step == 7) {
		for (int i = 0; i < 4; i++) {
			if (i == 2)
				continue;
			APUChannel *ch = &gb->apu.ch[i];
			if (!ch->env_period)
				continue;
			if (--ch->env_timer <= 0) {
				ch->env_timer = ch->env_period;
				if (ch->env_up && ch->volume < 15)
					ch->volume++;
				else if (!ch->env_up && ch->volume > 0)
					ch->volume--;
			}
		}
	}
}

static void step_channels(GB *gb, int cycles)
{
	for (int i = 0; i < 4; i++) {
		APUChannel *ch = &gb->apu.ch[i];
		ch->timer -= cycles;
		while (ch->timer <= 0) {
			switch (i) {
			case 0: case 1:
				ch->timer += square_period(ch);
				ch->duty_pos = (ch->duty_pos + 1) & 7;
				break;
			case 2:
				ch->timer += (2048 - ch->freq) * 2;
				ch->wave_pos = (ch->wave_pos + 1) & 31;
				break;
			default: {
				uint8_t nr43 = NR(0x22);
				int div = (nr43 & 7) ? (nr43 & 7) * 16 : 8;
				ch->timer += div << (nr43 >> 4);
				int bit = (ch->lfsr ^ (ch->lfsr >> 1)) & 1;
				ch->lfsr = (uint16_t)((ch->lfsr >> 1) | (bit << 14));
				if (nr43 & 0x08)
					ch->lfsr = (uint16_t)((ch->lfsr & ~0x40) | (bit << 6));
				break;
			}
			}
		}
	}
}

static int channel_output(GB *gb, int i)
{
	APUChannel *ch = &gb->apu.ch[i];
	if (!ch->on || !ch->dac)
		return 0;
	switch (i) {
	case 0: case 1:
		return (duty_table[ch->duty] >> (7 - ch->duty_pos)) & 1 ? ch->volume : 0;
	case 2: {
		static const int shift[4] = {4, 0, 1, 2};
		uint8_t b = NR(0x30 + ch->wave_pos / 2);
		int s = (ch->wave_pos & 1) ? (b & 0xf) : (b >> 4);
		return s >> shift[(NR(0x1c) >> 5) & 3];
	}
	default:
		return (ch->lfsr & 1) ? 0 : ch->volume;
	}
}

static void emit_sample(GB *gb)
{
	if (gb->apu.buffer_len >= AUDIO_BUFFER_FRAMES)
		return;
	int left = 0, right = 0;
	uint8_t nr51 = NR(0x25), nr50 = NR(0x24);
	for (int i = 0; i < 4; i++) {
		int out = channel_output(gb, i);
		if (nr51 & (1 << i))
			right += out;
		if (nr51 & (1 << (i + 4)))
			left += out;
	}
	left *= ((nr50 >> 4) & 7) + 1;
	right *= (nr50 & 7) + 1;

	// Max is 4 channels * 15 * 8 = 480.
	float l = (float)left / 480.0f, r = (float)right / 480.0f;
	float ol = l - hp_l, or_ = r - hp_r;
	hp_l = l - ol * 0.996f;
	hp_r = r - or_ * 0.996f;
	gb->apu.buffer[gb->apu.buffer_len * 2] = (int16_t)(ol * 24000.0f);
	gb->apu.buffer[gb->apu.buffer_len * 2 + 1] = (int16_t)(or_ * 24000.0f);
	gb->apu.buffer_len++;
}

// cycles are in real-time (single speed) T-cycles.
void apu_tick(GB *gb, int cycles)
{
	const double cycles_per_sample = (double)GB_CLOCK_HZ / AUDIO_SAMPLE_RATE;
	while (cycles > 0) {
		int step = cycles < 4 ? cycles : 4;
		cycles -= step;
		if (gb->apu.power) {
			step_channels(gb, step);
			gb->apu.frame_seq_timer += step;
			if (gb->apu.frame_seq_timer >= 8192) {
				gb->apu.frame_seq_timer -= 8192;
				frame_sequencer(gb);
			}
		}
		gb->apu.sample_acc += step;
		if (gb->apu.sample_acc >= cycles_per_sample) {
			gb->apu.sample_acc -= cycles_per_sample;
			emit_sample(gb);
		}
	}
}
