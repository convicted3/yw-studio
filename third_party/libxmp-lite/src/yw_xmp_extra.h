/* YW addition (round 190nw): see yw_xmp_extra.c. */
#ifndef YW_XMP_EXTRA_H
#define YW_XMP_EXTRA_H

#include "../include/libxmp-lite/xmp.h"

#ifdef __cplusplus
extern "C" {
#endif

double yw_xmp_sample_rate(xmp_context ctx, int smp);
double yw_xmp_frame_ms(xmp_context ctx);
int yw_xmp_period_type(xmp_context ctx);
int yw_xmp_sustain_loop(xmp_context ctx, int smp, int *start, int *end);
int yw_xmp_sample_ended(xmp_context ctx, int chn); /* round 190ob */
int yw_xmp_channel_pitch(xmp_context ctx, int chn, double *pitch, int *smp, double *pos); /* round 190om */
int yw_xmp_player_mode(xmp_context ctx); /* round 190om */

#ifdef __cplusplus
}
#endif

#endif
