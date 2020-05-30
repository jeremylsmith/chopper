/*
chopper - Split WAV files by silence
Copyright (C) 1999-2020 Jeremy Smith
-----
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
-----
INFORMATION
  www: https://github.com/jeremylsmith/chopper/
  e-mail: jeremy@decompiler.org
*/

#include <stdio.h>
#include <string.h>

FILE *hFile, *inp, *outp;

char item[6];

void
getitem ()
{
  item[0] = getc (inp);
  item[1] = getc (inp);
  item[2] = getc (inp);
  item[3] = getc (inp);
  item[4] = '\0';
};

unsigned long sampstart[500];
int playonesample, playwhichsample;
int messedcapital;
signed char onebytec, lastsamplec;
int scrollbarpos;
int akai, key, octave = 2, midi = 36, note = 0, keytype;
int lookforspacecount, ismultifile =
  0, lastslashposition, pathcount, multicount, fileextcount, slashposition,
  chopnum, loadresult;
int onlycount, sensitivity = 3, silencetolerance =
  500, noisetolerance, minsize = 8000;
int capitalcount, lastsample, difference;
int ticksilence = 0, tickoverwrite = 0, ticksmooth = 0;
int flagdontoverwrite = 0;
double getbpm (char *);
unsigned long wavfilelen, wavtype, samplerate, estbyps, bps, filelen,
  sizeofminute, chunklen, divideby, dividebpm, headerlen, highbpm,
  waveoutcount;
int stereo, dummy, countsplit, bps1, bps2, bp1, bp2, s1, s2, onebyte,
  outcount, silence, silentpass;
int silencecount, lastsilence;
unsigned long seeksilence;
char outname[512];
char filename[256];

void chopsilence (char *filename, char *onlypath, char *onlyfile);

unsigned long
bit32convl ()
{
  unsigned long d;
  unsigned char *ptr = (unsigned char *) &d;
  ptr[0] = getc (inp);
  ptr[1] = getc (inp);
  ptr[2] = getc (inp);
  ptr[3] = getc (inp);
  return (d);
};

unsigned long
bit32conv ()
{
  int getbyte;
  unsigned long d;
  unsigned char *ptr = (unsigned char *) &d;

  ptr[3] = getc (inp);
  ptr[2] = getc (inp);
  ptr[1] = getc (inp);
  ptr[0] = getc (inp);
  return (d);
};


int estchop;
char filepath[512], swapfilename[512], thismultifile[512], pathwith[512],
  message[512], fileext[512], chopnumstr[512];

int
main (int argc, char *argv[])
{
  if (argc != 2)
    {
      printf ("No file specified\n");
      return 1;
    }
  FILE *temp;
  temp = fopen (argv[1], "rb");
  if (!temp)
    {
      printf ("Can't open input file %s\n", argv[1]);
      return 1;
    }
  fclose (temp);
  getbpm (argv[1]);
//      chopsilence("C:/Data/MySoftware/github/chopper/601_COOK.WAV","C:/Data/MySoftware/github/chopper/","601_COOK.WAV");
  chopsilence (argv[1], "", argv[1]);
  return 0;
}

void
put32l (unsigned long d)
{
  unsigned char *ptr = (unsigned char *) &d;

  putc (ptr[0], outp);
  putc (ptr[1], outp);
  putc (ptr[2], outp);
  putc (ptr[3], outp);
};

double
getbpm (char *filename)
{
  inp = fopen (filename, "rb");

  bit32conv ();

/*Get length of WAV file*/
  wavfilelen = bit32conv ();

/*Get .WAVEfmt bit*/
  bit32conv ();
  bit32conv ();

/*Get header length isn't all that important*/
  headerlen = bit32convl ();

/*Get how many channels it is - 2 is stereo, 1 is mono*/
  s1 = getc (inp);
  s2 = getc (inp);

/*Number of channels*/
  stereo = getc (inp);

  dummy = getc (inp);

/*Get samplerate - quite important*/
  samplerate = bit32convl ();

/*Estimated Bits per second = 8/16*samplerate*channels*/
  estbyps = bit32convl ();
  bp1 = getc (inp);
  bp2 = getc (inp);

/*Bits/bytes per second*/
  bps1 = getc (inp);
  bps2 = getc (inp);
  bps = bps1 + (bps2 * 256);

  fseek (inp, 0x14L + headerlen, SEEK_SET);
  getitem ();

  if (strcmp (item, "data") != 0)
    {
      fseek (inp, ftell (inp) + bit32convl (), SEEK_SET);
      getitem ();
      getitem ();
    };
  filelen = bit32convl ();


/*Start of silence check*/
  waveoutcount = 0;
  seeksilence = ftell (inp);
  silencecount = 0;
  lastsilence = 0;
  while ((waveoutcount < filelen / (bps / 8)) && (lastsilence == 0))
    {
      outcount = 0;
      while ((outcount < (bps / 8)) && (lastsilence == 0))
	{
	  onebyte = getc (inp);
	  if (outcount == (bps / 8) - 1)
	    {
	      if (onebyte < 5)
		silencecount++;
	      else
		lastsilence = 1;
	    };

//putc(onebyte,outp);
	  outcount++;
	};
      waveoutcount++;
    };
//seeksilence=ftell(inp);
/*End of silence check*/

  sizeofminute = (bps / 8) * (stereo) * (samplerate) * 60;

/*Divideby is the amount we divide sample pointers by, to equal Cooledit's numbering system*/
  divideby = stereo * (bps / 8);

//headerlen=ftell(inp);
//chunklen=filelen/numbersplit;

  highbpm = sizeofminute / 240;
  dividebpm = 1L;
  while (1)
    {
      if (filelen / dividebpm < highbpm)
	{
	  break;
	};
      dividebpm *= 2L;
    };

  fclose (inp);
  estchop = dividebpm;
  return ((double) sizeofminute / ((double) filelen / dividebpm) /
	  (double) 2);
};




void
chopsilence (char *filename, char *onlypath, char *onlyfile)
{
  unsigned long beginning = 0, end = 0, length = 0, silencesize =
    0, endcount = 0;
  int endthisnow = 0;

  noisetolerance = 0x02;
//fclose(inp);
  countsplit = 1;
  inp = fopen (filename, "rb");
  fseek (inp, seeksilence, SEEK_SET);
  while (ftell (inp) - headerlen < filelen)
    {
      sampstart[countsplit] = ftell (inp);
/*Start of silence check*/
      waveoutcount = 0;
      silencecount = 0;
      lastsilence = 0;
      endcount = ftell (inp);
//while ((waveoutcount<filelen/(bps/8))&&(lastsilence==0))
      while (lastsilence == 0)
//while (silencecount<noisetolerance)
	{
	  if (endcount + (waveoutcount * (bps / 8)) > filelen)
	    {
//sprintf(message,"%d chops",countsplit-1);
//MessageBox(NULL,message,"Finished",NULL);
	      return;
	    };
	  outcount = 0;
	  while ((outcount < (bps / 8)) && (lastsilence == 0))
	    {
	      onebyte = getc (inp);
	      if (outcount == (bps / 8) - 1)
		{
		  if (lastsample > onebyte)
		    difference = lastsample - onebyte;
		  else
		    difference = onebyte - lastsample;
		  if (difference < sensitivity)
		    silencecount++;
		  else
		    {
//      sprintf(message,"%d",silencecount);
//      MessageBox(NULL,message,"",NULL);
//      silencecount++;
		      lastsilence = 1;
		    };
		  lastsample = onebyte;
		};

	      outcount++;
	    };

	  waveoutcount++;
	};
//fseek(inp,ftell(inp)-silencecount,SEEK_SET);
      beginning = ftell (inp);
/*End of silence check*/

      lastsilence = 0;
      silencecount = 0;
      waveoutcount = 0;
      endcount = ftell (inp);
/********************* yeah*/
      while (silencecount < silencetolerance)
	{
	  if (endcount + (waveoutcount * (bps / 8)) > filelen)
	    break;

	  outcount = 0;
	  while ((outcount < (bps / 8)) && (lastsilence == 0))
	    {
	      onebyte = getc (inp);
//if (ftell(inp)>filelen) return;
	      if (outcount == (bps / 8) - 1)
		{
		  if (lastsample > onebyte)
		    difference = lastsample - onebyte;
		  else
		    difference = onebyte - lastsample;
		  if (difference < sensitivity)
		    silencecount++;
		  if (difference >= sensitivity)
		    {
		      silencecount = 0;
		    };
		  lastsample = onebyte;
		};
	      outcount++;
	    };
	  waveoutcount++;
	};

      end = ftell (inp) - silencetolerance;
      length = end - beginning;

      fseek (inp, beginning, SEEK_SET);
      if (length + ftell (inp) + silencetolerance > filelen)
	{
	  endthisnow = 1;
	  length = filelen - ftell (inp);
	};
//      break;
      sprintf (outname, "%s%sx%02d.wav", onlypath, onlyfile, countsplit);

      if (tickoverwrite == 1)
	{
	  if ((hFile = fopen (outname, "rb")) != NULL)
	    {
	      fclose (hFile);
	      flagdontoverwrite = 1;
	    }
	  else
	    flagdontoverwrite = 0;
	};

      if (onlycount == 1)
	flagdontoverwrite = 1;

      if (length < minsize)
	{
	  flagdontoverwrite = 1;
	  countsplit--;
	};

      if (flagdontoverwrite == 0)
	{
	  printf ("Writing WAV file %s\n", outname);
	  outp = fopen (outname, "wb");

	  fprintf (outp, "RIFF");
	  put32l (headerlen + length + 0x34);
	  fprintf (outp, "WAVE");
	  fprintf (outp, "fmt ");
	  put32l (headerlen);
	  putc (s1, outp);
	  putc (s2, outp);
	  putc (stereo, outp);
	  putc (0, outp);
	  put32l (samplerate);
	  put32l (estbyps);
	  putc (bp1, outp);
	  putc (bp2, outp);
	  putc (bps1, outp);
	  putc (bps2, outp);
	  fprintf (outp, "data");
	  put32l (length);
	};

      silence = 25;
      silentpass = 0;

      waveoutcount = 0;
      while (waveoutcount < length / (bps / 8))
	{
	  outcount = 0;

	  while (outcount < (bps / 8))
	    {
	      onebyte = getc (inp);
	      if ((outcount == (bps / 8) - 1) && (ticksmooth == 1))
		{
		  lastsample = onebyte;
		  if ((silence > 0) && (silentpass == 1))
		    {
		      difference = (signed int) onebyte - lastsample;
		      difference /= silence;
		      onebyte = (signed int) 0 - difference + silence;

		      //onebyte=
//if (onebyte>64) onebyte
		      //onebyte-=silence+255;
		      silence++;
		      //if (onebyte<127) onebyte=;
		      if (silence == 25)
			silentpass = 2;
		    };

		  if ((silence != 0) && (silentpass == 0))
		    {
		      difference = (signed int) onebyte - lastsample;
		      difference /= silence;
		      onebyte = (signed int) 0 + difference + silence;

//difference=(signed int)onebyte-lastsample;
//onebyte=(signed int)0+difference-silence;
//      onebyte-=silence;
		      silence--;
//      if (onebyte<127) onebyte=127;
		      if (silence == 0)
			silentpass = 1;
		    };
		};

	      if (flagdontoverwrite == 0)
		putc (onebyte, outp);
	      outcount++;
	    };
	  waveoutcount++;
	  if (waveoutcount == (chunklen - 25) / (bps / 8))
	    {
	      silence = 25;
	    };
	};
      if (flagdontoverwrite == 0)
	fclose (outp);
      countsplit++;

//fclose(inp);
      flagdontoverwrite = 0;
      if (endthisnow == 1)
	break;
      fseek (inp, end, SEEK_SET);

//if (end+0x500>filelen) break;
/******************************************here*/
    };
  fclose (inp);

  chopnum = countsplit - 1;
};

