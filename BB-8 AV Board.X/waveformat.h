/* 
 * File:   waveformat.h
 * Author: Corey
 *
 * Created on September 18, 2015, 11:52 PM
 */

#ifndef WAVEFORMAT_H
#define	WAVEFORMAT_H

#ifdef	__cplusplus
extern "C" {
#endif

    enum WaveFormats
    {
        PCM = 1,
        FLOAT = 3
    };

    typedef struct _WaveFormat {
        char ChunkID[4]; // RIFF
        DWORD ChunkSize;
        char Format[4]; // WAVE
        char Subchunk1ID[4]; // "fmt "
        DWORD Subchunk1Size;
        short AudioFormat; // PCM, Float
        short NumberChannels;
        DWORD SampleRate;
        DWORD ByteRate;
        short BlockAlign;
        short BitsPerSample;
        // variable number of bits here, below is not guaranteed
        //char Subchunk2ID[4]; // data
        //DWORD Subchunk2Size;

    } WaveFormat;

    typedef struct _DataChunk {
        char Subchunk2ID[4]; // data
        DWORD Subchunk2Size;
    } DataChunk;


#ifdef	__cplusplus
}
#endif

#endif	/* WAVEFORMAT_H */

