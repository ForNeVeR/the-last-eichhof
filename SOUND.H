// SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

// Header file for sound support module.


#define SND_PACKED4		0x0001
struct sndstrc {

   int    priority;			// Priority of sample.
   int    samplerate;			// Sample rate in kHz.
   int    flags;			// Raw or packed sample?
   long   len;				// Sample len in bytes.
   char   data[];			// Sampled data.

};


int initsound(int snd_io, int snd_irq, int snd_dma);
void shutsound(void);
void playsample(struct sndstrc far *snd);
void playloop(struct sndstrc far *snd);
void playfile(int filvar, void far *buffer, int bs);
void playfileloop(int filvar, void far *buffer, int bs);
void snd_cli(void);
void snd_sti(void);
void haltsound(void);
int soundbusy(void);
int speaker(int state);

