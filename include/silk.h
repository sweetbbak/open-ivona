/* SILK SDK 1.0.8 decoder API (statically linked, verified as lib:silk; interface/SKP_Silk_*.h). */
#ifndef IVONA_SILK_H
#define IVONA_SILK_H

typedef struct {
    int API_sampleRate;             /* I: output rate, 8000/12000/16000/24000 */
    int frameSize;                  /* O: samples per frame */
    int framesPerPacket;            /* O: frames per packet, 1..5 */
    int moreInternalDecoderFrames;  /* O: the decoder holds more frames of this packet */
    int inBandFECOffset;            /* O */
} SKP_SILK_SDK_DecControlStruct;

int SKP_Silk_SDK_Get_Decoder_Size(int *decSizeBytes);
int SKP_Silk_SDK_InitDecoder(void *decState);
int SKP_Silk_SDK_Decode(void *decState, SKP_SILK_SDK_DecControlStruct *decControl, int lostFlag,
                        const unsigned char *inData, const int nBytesIn, short *samplesOut,
                        short *nSamplesOut);

#endif
