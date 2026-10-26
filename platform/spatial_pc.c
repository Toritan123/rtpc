/*
 * Spatial output (pc-enhanced branch), settings.ini `spatial`:
 *
 *  headphones  The two channels are played as two virtual speakers at +-30
 *              degrees in front of the listener, instead of straight into
 *              each ear. Each speaker reaches the far ear later and duller,
 *              as it would in a room: the delay and the head-shadow filter
 *              come from Brown & Duda's spherical head model (1998) -- head
 *              radius 8.75 cm, interaural delay from the Woodworth formula,
 *              a one-pole/one-zero shelf per ear whose high-frequency gain
 *              alpha(theta) rises towards the near side and falls behind --
 *              so no measured HRTF set is needed. Hard-panned sounds stop
 *              sitting inside one ear.
 *              It is built as a shuffler: the mid (L+R) passes unchanged and
 *              only the side (L-R) is filtered, by the ratio of the model's
 *              side and mid responses, (Hn - s Hf D) / (Hn + s Hf D) (near
 *              shelf Hn, far shelf Hf, ITD delay D, strength s). The two
 *              ears then differ exactly as the model says for any panned
 *              sound, while a centred sound is not coloured at all -- the
 *              direct form left the comb of near + delayed far path in it,
 *              -1 to -2 dB across 0.2-4 kHz.
 *  wide        A wider stereo image for speakers: the side (L-R) signal is
 *              raised above ~200 Hz, the bass stays centred.
 *  off         Unchanged (default).
 *
 * spatial_strength (0-100, default 60) scales the effect: the far-ear path
 * in headphones mode, the side boost in wide mode. Both keep a mono
 * (centred) sound at the same low-frequency level.
 */
#ifdef PLATFORM_PC

#include <math.h>
#include <stdlib.h>
#include <string.h>

extern const char *pc_settings_get(const char *key);

#define SPK_DEG     30.0
#define HEAD_RADIUS 0.0875
#define SOUND_SPEED 343.0
#define RING        64

enum { SP_OFF, SP_HEADPHONES, SP_WIDE };

struct Shelf { double b0, b1, a1, x1, y1; };

static int    s_mode = -1, s_rate;
static double s_strength = 0.6;
// headphones: near shelf on the side, far shelves on the delayed side and
// on the delayed output (the feedback that divides by the mid response),
// and the inverse near shelf.
static struct Shelf s_near, s_far_ff, s_far_fb, s_near_inv;
static float  s_side_in[RING], s_side_out[RING];
static int    s_rpos;
static double s_itd;                  // samples
static double s_side_lp[1], s_side_a; // wide: low-passed side, to keep the bass centred

// Brown & Duda's head-shadow filter for an ear at angle theta (degrees,
// 0 = source on the ear's own axis, 180 = directly opposite), bilinear.
static double shelf_alpha(double theta)
{
    const double amin = 0.1, tmin = 150.0;
    return (1.0 + amin / 2.0) + (1.0 - amin / 2.0) * cos(theta / tmin * M_PI);
}

// The shelf with a given high-frequency gain alpha (DC gain 1).
static void shelf_init_alpha(struct Shelf *f, double alpha, int rate)
{
    double w0 = SOUND_SPEED / HEAD_RADIUS;
    double K = 2.0 * rate, beta = 1.0 / (2.0 * w0);
    double a0 = 1.0 + beta * K;
    f->b0 = (1.0 + alpha * beta * K) / a0;
    f->b1 = (1.0 - alpha * beta * K) / a0;
    f->a1 = (1.0 - beta * K) / a0;
    f->x1 = f->y1 = 0.0;
}

static inline double shelf(struct Shelf *f, double x)
{
    double y = f->b0 * x + f->b1 * f->x1 - f->a1 * f->y1;
    f->x1 = x; f->y1 = y;
    return y;
}

// Woodworth / Brown-Duda delay for an ear at angle theta, seconds.
static void shelf_init(struct Shelf *f, double theta, int rate)
{
    shelf_init_alpha(f, shelf_alpha(theta), rate);
}

static double ear_delay(double theta)
{
    double t = theta * M_PI / 180.0;
    return (theta < 90.0) ? -HEAD_RADIUS / SOUND_SPEED * cos(t)
                          : HEAD_RADIUS / SOUND_SPEED * (t - M_PI / 2.0);
}

static void setup(int rate)
{
    const char *m = pc_settings_get("spatial");
    int c;
    s_mode = !strcmp(m, "headphones") ? SP_HEADPHONES : !strcmp(m, "wide") ? SP_WIDE : SP_OFF;
    s_strength = atof(pc_settings_get("spatial_strength")) / 100.0;
    if (s_strength < 0.0) s_strength = 0.0;
    if (s_strength > 1.0) s_strength = 1.0;
    s_rate = rate;
    // A speaker at +30 degrees is 60 degrees off the right ear's axis and
    // 120 degrees off the left's.
    shelf_init(&s_near, 90.0 - SPK_DEG, rate);
    shelf_init(&s_far_ff, 90.0 + SPK_DEG, rate);
    shelf_init(&s_far_fb, 90.0 + SPK_DEG, rate);
    {   // inverse of the near shelf: swap numerator and denominator
        struct Shelf n = s_near;
        s_near_inv.b0 = 1.0 / n.b0;
        s_near_inv.b1 = n.a1 / n.b0;
        s_near_inv.a1 = n.b1 / n.b0;
        s_near_inv.x1 = s_near_inv.y1 = 0.0;
    }
    s_itd = (ear_delay(90.0 + SPK_DEG) - ear_delay(90.0 - SPK_DEG)) * rate;
    memset(s_side_in, 0, sizeof(s_side_in));
    memset(s_side_out, 0, sizeof(s_side_out));
    s_rpos = 0;
    s_side_lp[0] = 0.0;
    s_side_a = exp(-2.0 * M_PI * 200.0 / rate);
}

void spatial_pc_reconfigure(void)
{
    s_mode = -1;
}

void spatial_pc_process(float *buf, unsigned frames, int rate)
{
    unsigned i;
    if (s_mode < 0 || rate != s_rate) setup(rate);
    if (s_mode == SP_OFF || s_strength == 0.0) return;

    if (s_mode == SP_WIDE) {
        double boost = 0.8 * s_strength;   // up to +5 dB of side above 200 Hz
        for (i = 0; i < frames; i++) {
            double l = buf[2 * i], r = buf[2 * i + 1];
            double mid = 0.5 * (l + r), side = 0.5 * (l - r), hi;
            s_side_lp[0] = s_side_a * s_side_lp[0] + (1.0 - s_side_a) * side;
            hi = side - s_side_lp[0];
            side += boost * hi;
            buf[2 * i]     = (float)(mid + side);
            buf[2 * i + 1] = (float)(mid - side);
        }
        return;
    }

    // Headphones (shuffler): mid unchanged; side y solves
    //   Hn y + s Hf D y = Hn x - s Hf D x
    // i.e. y = Hn^-1 (Hn x - s Hf D x - s Hf D y). The loop gain
    // |s Hf / Hn| is at most s < 1, so it is stable.
    {
        int di = (int)s_itd;
        double fr = s_itd - di;
        for (i = 0; i < frames; i++) {
            double l = buf[2 * i], r = buf[2 * i + 1];
            double mid = 0.5 * (l + r), x = 0.5 * (l - r), y, v;
            int p0 = (s_rpos - di + RING) % RING, p1 = (s_rpos - di - 1 + RING) % RING;
            double xd, yd;
            s_side_in[s_rpos] = (float)x;
            xd = s_side_in[p0] * (1.0 - fr) + s_side_in[p1] * fr;
            yd = s_side_out[p0] * (1.0 - fr) + s_side_out[p1] * fr;
            v = shelf(&s_near, x) - s_strength * shelf(&s_far_ff, xd) - s_strength * shelf(&s_far_fb, yd);
            y = shelf(&s_near_inv, v);
            s_side_out[s_rpos] = (float)y;
            s_rpos = (s_rpos + 1) % RING;
            buf[2 * i]     = (float)(mid + y);
            buf[2 * i + 1] = (float)(mid - y);
        }
    }
}

#endif
