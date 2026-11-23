// Round 95: YW Tune (own real-time vocal pitch corrector) and the shared vocal pitch toolkit (core/VocalPitch.h).
// Synthetic "vocals" only: a band-limited glottal-like source (harmonics falling 1/k^1.3) through three formant
// resonators (700 / 1220 / 2600 Hz), so pitch accuracy can be measured against a known truth.

#include <atomic>
#include <cstdlib>
#include <new>

#include "core/YwTune.h"
#include "core/VocalPitch.h"

// Allocation counter for the real-time tests: counts only while `countAllocs` is set (the audio-thread calls).
namespace rt95
{
std::atomic<long> allocs { 0 };
std::atomic<bool> countAllocs { false };
} // namespace rt95

void* operator new (std::size_t n)
{
    if (rt95::countAllocs.load (std::memory_order_relaxed))
        rt95::allocs.fetch_add (1, std::memory_order_relaxed);
    if (auto* p = std::malloc (n > 0 ? n : 1))
        return p;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t n) { return operator new (n); }
void* operator new (std::size_t n, const std::nothrow_t&) noexcept
{
    if (rt95::countAllocs.load (std::memory_order_relaxed))
        rt95::allocs.fetch_add (1, std::memory_order_relaxed);
    return std::malloc (n > 0 ? n : 1);
}
void* operator new[] (std::size_t n, const std::nothrow_t& t) noexcept { return operator new (n, t); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }
void operator delete (void* p, const std::nothrow_t&) noexcept { std::free (p); }
void operator delete[] (void* p, const std::nothrow_t&) noexcept { std::free (p); }

namespace r95
{
constexpr double pi = 3.14159265358979323846;

// f0(t) in Hz → a synthetic vowel
std::vector<float> vowel (double fs, int n, const std::function<double (double)>& f0, double f1 = 700.0, double f2 = 1220.0, double f3 = 2600.0)
{
    std::vector<float> y ((std::size_t) n);
    struct Res
    {
        double a1, a2, g, z1 = 0, z2 = 0;
    };
    auto res = [fs] (double f, double bw) {
        const auto r = std::exp (-pi * bw / fs);
        return Res { -2.0 * r * std::cos (2.0 * pi * f / fs), r * r, 1.0 - r };
    };
    Res rs[3] { res (f1, 80.0), res (f2, 90.0), res (f3, 120.0) };
    double ph = 0.0;
    unsigned seed = 1;
    for (int i = 0; i < n; ++i)
    {
        const auto f = f0 (i / fs);
        ph += f / fs;
        ph -= std::floor (ph);
        double x = 0.0;
        const auto harmonics = std::min (80, (int) (fs * 0.45 / f));
        for (int k = 1; k <= harmonics; ++k)
            x += std::sin (2.0 * pi * k * ph) / std::pow (k, 1.3);
        seed = seed * 1664525u + 1013904223u;
        x += 1e-3 * ((seed >> 8) / 16777216.0 - 0.5);
        double o = 0.0;
        for (auto& r : rs)
        {
            const auto v = r.g * x - r.a1 * r.z1 - r.a2 * r.z2;
            r.z2 = r.z1;
            r.z1 = v;
            o += v;
        }
        y[(std::size_t) i] = (float) (0.3 * o);
    }
    return y;
}

// An independent pitch measure for the tests (McLeod's normalised square difference, first key maximum ≥ 0.9·max)
double measureHz (const std::vector<float>& x, int from, int len, double fs, double fmin = 60.0, double fmax = 1200.0)
{
    const auto tmin = (int) (fs / fmax), tmax = (int) (fs / fmin);
    if (from < 0 || from + len + tmax + 2 > (int) x.size())
        return 0.0;
    std::vector<double> nsdf ((std::size_t) tmax + 2, 0.0);
    for (int t = 0; t <= tmax + 1; ++t)
    {
        double r = 0.0, m = 0.0;
        for (int j = from; j < from + len - t; ++j)
        {
            r += (double) x[(std::size_t) j] * x[(std::size_t) (j + t)];
            m += (double) x[(std::size_t) j] * x[(std::size_t) j] + (double) x[(std::size_t) (j + t)] * x[(std::size_t) (j + t)];
        }
        nsdf[(std::size_t) t] = m > 0.0 ? 2.0 * r / m : 0.0;
    }
    double top = 0.0;
    for (int t = tmin; t <= tmax; ++t)
        top = std::max (top, nsdf[(std::size_t) t]);
    for (int t = std::max (1, tmin); t <= tmax; ++t)
        if (nsdf[(std::size_t) t] >= 0.9 * top && nsdf[(std::size_t) t] >= nsdf[(std::size_t) t - 1] && nsdf[(std::size_t) t] >= nsdf[(std::size_t) t + 1])
        {
            const auto a = nsdf[(std::size_t) t - 1], b = nsdf[(std::size_t) t], c = nsdf[(std::size_t) t + 1];
            const auto d = a - 2.0 * b + c;
            return fs / (t + (d != 0.0 ? 0.5 * (a - c) / d : 0.0));
        }
    return 0.0;
}

double cents (double hz, double refHz) { return 1200.0 * std::log2 (hz / refHz); }

// Where the first formants lie: the centroid of the power spectrum between 200 and 1800 Hz (Hann window, 10 Hz
// steps). A formant shift scales it; a pitch shift with the formants preserved leaves it in place.
double formantCentre (const std::vector<float>& x, int from, int len, double fs)
{
    double num = 0.0, den = 0.0;
    for (double f = 200.0; f <= 1800.0; f += 10.0)
    {
        double re = 0.0, im = 0.0;
        const auto w = 2.0 * pi * f / fs;
        for (int i = 0; i < len; ++i)
        {
            const auto v = x[(std::size_t) (from + i)] * (0.5 - 0.5 * std::cos (2.0 * pi * i / (len - 1)));
            re += v * std::cos (w * i);
            im -= v * std::sin (w * i);
        }
        num += f * (re * re + im * im);
        den += re * re + im * im;
    }
    return den > 0.0 ? num / den : 0.0;
}

yw::EffectParams tuneDefaults()
{
    yw::EffectParams p {};
    const auto& info = yw::effectInfo (yw::EffectKind::ywTune);
    for (std::size_t i = 0; i < info.params.size(); ++i)
        p[i] = info.params[i].defaultValue;
    return p;
}

// Runs YW Tune over a mono signal (both channels), in blocks of `block`
std::vector<float> runTune (const std::vector<float>& x, double fs, const yw::EffectParams& p, int& latency, int block = 256,
                            const yw::NoteMask* held = nullptr, int heldCount = 0)
{
    auto fx = yw::makeEffect (yw::EffectKind::ywTune);
    fx->prepare (fs);
    std::vector<float> l = x, r = x;
    for (int i = 0; i < (int) x.size(); i += block)
    {
        const auto n = std::min (block, (int) x.size() - i);
        if (held != nullptr)
            fx->setHeldNotes (held, heldCount);
        fx->process (l.data() + i, r.data() + i, n, p, 120.0);
    }
    latency = fx->latencySamples();
    return l;
}
} // namespace r95

TEST (vocal_pitch_detector_accuracy_round95)
{
    // YIN + full-rate refinement on steady synthetic vowels: within 1 cent at 44.1 / 48 / 96 kHz, 82 Hz … 700 Hz
    double worst = 0.0;
    int voicedShare = 0, frames = 0;
    for (const double fs : { 44100.0, 48000.0, 96000.0 })
        for (const double hz : { 82.0, 110.0, 196.0, 261.63, 440.0, 700.0 })
        {
            const auto x = r95::vowel (fs, (int) (fs * 0.6), [hz] (double) { return hz; });
            yw::vocal::PitchDetector det;
            det.prepare (fs, 60.0, 1000.0, (int) (fs * 0.003));
            double last = 0.0;
            for (const auto v : x)
                if (det.push (v))
                {
                    ++frames;
                    if (det.estimate().voiced)
                    {
                        ++voicedShare;
                        last = det.estimate().hz;
                    }
                }
            worst = std::max (worst, std::abs (r95::cents (last, hz)));
        }
    CHECK (worst < 1.0);
    CHECK (voicedShare == frames);

    // silence and noise are not voiced
    yw::vocal::PitchDetector det;
    det.prepare (44100.0, 60.0, 1000.0, 128);
    unsigned seed = 7;
    int voicedNoise = 0;
    for (int i = 0; i < 44100; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        const auto v = i < 22050 ? 0.0f : 0.3f * ((float) (seed >> 8) / 16777216.0f - 0.5f);
        if (det.push (v) && det.estimate().voiced)
            ++voicedNoise;
    }
    CHECK (voicedNoise == 0);

    // offline analysis (Newtone): frames every 256 samples, a glide 200 → 300 Hz followed within 3 cents
    const double fs = 44100.0;
    const auto glide = r95::vowel (fs, (int) fs, [] (double t) { return 200.0 * std::pow (1.5, t); });
    const auto track = yw::vocal::analysePitch (glide.data(), (int) glide.size(), fs, 256);
    double glideWorst = 0.0;
    for (std::size_t k = 20; k + 20 < track.size(); ++k)
        glideWorst = std::max (glideWorst, std::abs (r95::cents (track[k].hz, 200.0 * std::pow (1.5, (double) k * 256.0 / fs))));
    CHECK (glideWorst < 4.0);
}

TEST (vocal_scales_and_note_choice_round95)
{
    using yw::vocal::Scale;
    CHECK (yw::vocal::scaleMask (Scale::major, 0) == 0b101010110101);        // C D E F G A B
    CHECK (yw::vocal::scaleMask (Scale::minor, 9) == 0b101010110101);        // A minor = C major's notes
    CHECK (yw::vocal::scaleMask (Scale::chromatic, 5) == 0xfff);
    CHECK (yw::vocal::scaleMask (Scale::custom, 3, 0x81) == 0x81);
    CHECK (yw::vocal::scaleMask (Scale::majorPentatonic, 7) == ((1 << 7) | (1 << 9) | (1 << 11) | (1 << 2) | (1 << 4)));

    const auto cMajor = yw::vocal::scaleMask (Scale::major, 0);
    CHECK (yw::vocal::nearestNote (61.4, cMajor) == 62);  // C#+40 → D
    CHECK (yw::vocal::nearestNote (61.4, 0xfff) == 61);   // chromatic → C#
    CHECK (yw::vocal::nearestNote (60.45, 0xfff) == 60);
    // hysteresis: a note wobbling just past the midpoint stays on its current target
    CHECK (yw::vocal::nearestNote (60.55, 0xfff, 60, 0.1) == 60);
    CHECK (yw::vocal::nearestNote (60.65, 0xfff, 60, 0.1) == 61);
    CHECK (std::abs (yw::vocal::hzToNote (440.0) - 69.0) < 1e-12);
    CHECK (std::abs (yw::vocal::noteToHz (60.0, 432.0) - 432.0 * std::exp2 (-9.0 / 12.0)) < 1e-9);
}

TEST (yw_tune_corrects_to_the_scale_round95)
{
    const double fs = 44100.0;
    // sung: A3 +30 cents, C4 −40 cents, C#4 +40 cents (between C# and D), each 0.8 s, no vibrato
    const double sung[3] { 220.0 * std::exp2 (30.0 / 1200.0), 261.6256 * std::exp2 (-40.0 / 1200.0), 277.1826 * std::exp2 (40.0 / 1200.0) };
    const auto x = r95::vowel (fs, (int) (fs * 2.4), [&] (double t) { return sung[std::min (2, (int) (t / 0.8))]; });

    auto p = r95::tuneDefaults();
    p[yw::ywtune::retune] = 0.0f; // instant
    int latency = 0;
    const auto y = r95::runTune (x, fs, p, latency);
    CHECK (latency == yw::ywtune::latencyFor (fs, 1, false));

    // C major: A3, C4, D4 within 3 cents
    const double want[3] { 220.0, 261.6256, 293.6648 };
    double worst = 0.0;
    for (int k = 0; k < 3; ++k)
        for (const double t : { 0.3, 0.5 })
            worst = std::max (worst, std::abs (r95::cents (r95::measureHz (y, (int) ((k * 0.8 + t) * fs) + latency, 2048, fs), want[k])));
    CHECK (worst < 3.0);

    // chromatic: the third note goes to C#4 instead
    p[yw::ywtune::scale] = 0.0f;
    const auto z = r95::runTune (x, fs, p, latency);
    CHECK (std::abs (r95::cents (r95::measureHz (z, (int) (2.0 * fs) + latency, 2048, fs), 277.1826)) < 3.0);

    // a note switched off is never a target: without D the third note goes to C
    p[yw::ywtune::scale] = 1.0f / (float) (yw::vocal::scaleCount - 1);
    p[yw::ywtune::noteC + 2] = 0.0f;
    const auto w = r95::runTune (x, fs, p, latency);
    CHECK (std::abs (r95::cents (r95::measureHz (w, (int) (2.0 * fs) + latency, 2048, fs), 261.6256)) < 3.0);

    // key and reference: E major, A4 = 432 Hz — the first note (A3 at 440 +30 c) goes to that tuning's A3
    p = r95::tuneDefaults();
    p[yw::ywtune::retune] = 0.0f;
    p[yw::ywtune::key] = 4.0f / 11.0f;
    p[yw::ywtune::reference] = (432.0f - 430.0f) / 20.0f;
    const auto v = r95::runTune (x, fs, p, latency);
    CHECK (std::abs (r95::cents (r95::measureHz (v, (int) (0.4 * fs) + latency, 2048, fs), 216.0)) < 3.0);

    // transpose +2 on top of the correction; at 48 kHz and in low-latency mode too
    p = r95::tuneDefaults();
    p[yw::ywtune::retune] = 0.0f;
    p[yw::ywtune::transpose] = 14.0f / 24.0f;
    p[yw::ywtune::lowLatency] = 1.0f;
    const auto x48 = r95::vowel (48000.0, 48000, [&] (double) { return sung[0]; });
    const auto u = r95::runTune (x48, 48000.0, p, latency);
    CHECK (latency == yw::ywtune::latencyFor (48000.0, 1, true) && latency < (int) (0.016 * 48000.0));
    CHECK (std::abs (r95::cents (r95::measureHz (u, 24000 + latency, 2048, 48000.0), 220.0 * std::exp2 (2.0 / 12.0))) < 3.0);
}

TEST (yw_tune_retune_speed_and_humanize_round95)
{
    // A3 +25 cents with a 5.5 Hz ±25 cent vibrato: instant retune flattens it, slow retune keeps it
    const double fs = 44100.0;
    const auto x = r95::vowel (fs, (int) (fs * 2.0), [] (double t) { return 220.0 * std::exp2 ((25.0 + 25.0 * std::sin (2.0 * r95::pi * 5.5 * t)) / 1200.0); });
    auto spread = [&] (const std::vector<float>& y, int latency) {
        double lo = 1e9, hi = -1e9;
        for (double t = 0.6; t < 1.8; t += 0.02)
        {
            const auto c = r95::cents (r95::measureHz (y, (int) (t * fs) + latency, 1024, fs, 150.0, 400.0), 220.0);
            lo = std::min (lo, c);
            hi = std::max (hi, c);
        }
        return hi - lo;
    };
    const auto inSpread = spread (x, 0);
    auto p = r95::tuneDefaults();
    int latency = 0;
    p[yw::ywtune::retune] = 0.0f;
    const auto hard = spread (r95::runTune (x, fs, p, latency), latency);
    p[yw::ywtune::retune] = 1.0f; // 400 ms
    const auto slow = spread (r95::runTune (x, fs, p, latency), latency);
    p[yw::ywtune::retune] = 0.1f; // 4 ms
    p[yw::ywtune::humanize] = 1.0f; // (a sustained note would relax by +135 ms)
    const auto human = spread (r95::runTune (x, fs, p, latency), latency);
    p[yw::ywtune::humanize] = 0.0f;
    const auto fast = spread (r95::runTune (x, fs, p, latency), latency);
    CHECK (inSpread > 40.0);
    CHECK (hard < 0.2 * inSpread);
    CHECK (slow > 0.6 * inSpread);
    // round 120: humanize slows only sustained (steady) notes, as measured on Auto-Tune Pro — a vibrato note is not one
    CHECK (human < fast * 1.5 + 2.0);
}

TEST (yw_tune_formants_round95)
{
    // two semitones up: with formant preserve the first formant stays (±5 %); without it moves up with the pitch
    const double fs = 44100.0;
    const auto x = r95::vowel (fs, (int) (fs * 1.5), [] (double) { return 164.8138; }); // E3, on its note
    auto p = r95::tuneDefaults();
    p[yw::ywtune::retune] = 0.0f;
    p[yw::ywtune::scale] = 0.0f;
    p[yw::ywtune::transpose] = 14.0f / 24.0f;
    int latency = 0;
    const auto f1In = r95::formantCentre (x, 30000, 4096, fs);
    const auto kept = r95::runTune (x, fs, p, latency);
    const auto f1Kept = r95::formantCentre (kept, 30000 + latency, 4096, fs);
    p[yw::ywtune::formantPreserve] = 0.0f;
    const auto moved = r95::runTune (x, fs, p, latency);
    const auto f1Moved = r95::formantCentre (moved, 30000 + latency, 4096, fs);
    p[yw::ywtune::formantPreserve] = 1.0f;
    p[yw::ywtune::formantShift] = 0.5f - 3.0f / 14.0f; // −3 semitones
    const auto lower = r95::runTune (x, fs, p, latency);
    const auto f1Lower = r95::formantCentre (lower, 30000 + latency, 4096, fs);
    CHECK (std::abs (f1Kept / f1In - 1.0) < 0.06);
    CHECK (f1Moved / f1In > 1.06);
    CHECK (f1Lower / f1In < 0.94);
    // the pitch is the same in the three
    CHECK (std::abs (r95::cents (r95::measureHz (moved, 30000 + latency, 2048, fs), 184.9972)) < 3.0); // F#3
}

TEST (yw_tune_midi_targets_round95)
{
    // the target notes come from the notes held on the chosen channel (pitch classes, any octave)
    const double fs = 44100.0;
    const auto x = r95::vowel (fs, (int) fs, [] (double) { return 220.0 * std::exp2 (60.0 / 1200.0); }); // A3 +60 c
    auto p = r95::tuneDefaults();
    p[yw::ywtune::retune] = 0.0f;
    p[yw::ywtune::midiChannel] = 3.0f / 128.0f; // channel 3 (index 2)
    std::array<yw::NoteMask, 4> held {};
    held[2][1] |= std::uint64_t { 1 } << (82 - 64); // A#5 held: A#3 is the target
    int latency = 0;
    const auto y = r95::runTune (x, fs, p, latency, 256, held.data(), (int) held.size());
    CHECK (std::abs (r95::cents (r95::measureHz (y, 22050 + latency, 2048, fs), 233.0819)) < 3.0);
    // nothing held there: the scale again (A3)
    std::array<yw::NoteMask, 4> none {};
    const auto z = r95::runTune (x, fs, p, latency, 256, none.data(), (int) none.size());
    CHECK (std::abs (r95::cents (r95::measureHz (z, 22050 + latency, 2048, fs), 220.0)) < 3.0);
    CHECK (yw::formatEffectParam (yw::EffectKind::ywTune, yw::ywtune::midiChannel, 3.0f / 128.0f, 120.0) == "Channel 3");
    CHECK (yw::formatEffectParam (yw::EffectKind::ywTune, yw::ywtune::midiChannel, 0.0f, 120.0) == "Off");
}

TEST (yw_tune_transparency_and_realtime_round95)
{
    // a voice already on its notes, mixed with unvoiced noise, comes out as itself delayed by the latency
    const double fs = 44100.0;
    auto x = r95::vowel (fs, (int) fs, [] (double) { return 196.0; });
    unsigned seed = 3;
    for (int i = 30000; i < 34000; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        x[(std::size_t) i] = 0.1f * ((float) (seed >> 8) / 16777216.0f - 0.5f); // a consonant
    }
    auto p = r95::tuneDefaults();
    int latency = 0;
    const auto y = r95::runTune (x, fs, p, latency);
    auto residualDb = [&] (int a, int b) {
        double err = 0.0, ref = 0.0;
        for (int i = a; i < b; ++i)
        {
            const auto e = (double) y[(std::size_t) (i + latency)] - x[(std::size_t) i];
            err += e * e;
            ref += (double) x[(std::size_t) i] * x[(std::size_t) i];
        }
        return 10.0 * std::log10 (err / ref);
    };
    CHECK (residualDb (8000, 29500) < -50.0);  // the voice
    CHECK (residualDb (30500, 33500) < -60.0); // the consonant (plain overlap-add)
    CHECK (residualDb (38500, 42000) < -50.0); // the voice again, 100 ms after its onset

    // dry mix 0 = the input delayed exactly; non-finite input does not poison the state
    p[yw::ywtune::mix] = 0.0f;
    const auto d = r95::runTune (x, fs, p, latency);
    bool exact = true;
    for (int i = 0; i + latency < (int) x.size(); ++i)
        exact = exact && d[(std::size_t) (i + latency)] == x[(std::size_t) i];
    CHECK (exact);

    // the audio thread never allocates: any block size, every parameter moving, voice / silence / NaN
    auto fx = yw::makeEffect (yw::EffectKind::ywTune);
    fx->prepare (48000.0);
    auto q = r95::tuneDefaults();
    std::vector<float> l (4096), r (4096);
    std::array<yw::NoteMask, 2> held {};
    held[1][0] = 0x10;
    const auto voice = r95::vowel (48000.0, 4096 * 8, [] (double t) { return 150.0 + 100.0 * t; });
    rt95::allocs.store (0);
    rt95::countAllocs.store (true);
    bool finite = true;
    for (int b = 0; b < 8; ++b)
    {
        for (int i = 0; i < 4096; ++i)
            l[(std::size_t) i] = r[(std::size_t) i] = voice[(std::size_t) (b * 4096 + i)];
        if (b == 5)
            l[100] = std::numeric_limits<float>::quiet_NaN();
        for (int k = 0; k < yw::ywtune::count; ++k)
            q[(std::size_t) k] = (float) ((b * 7 + k * 3) % 11) / 10.0f;
        fx->setHeldNotes (held.data(), 2);
        const int sizes[] { 1, 17, 256, 3822 };
        for (int at = 0, s = 0; at < 4096; at += sizes[s % 4], ++s)
            fx->process (l.data() + at, r.data() + at, std::min (sizes[s % 4], 4096 - at), q, 120.0);
        if (b >= 7)
            for (int i = 0; i < 4096; ++i)
                finite = finite && std::isfinite (l[(std::size_t) i]) && std::isfinite (r[(std::size_t) i]);
    }
    rt95::countAllocs.store (false);
    CHECK (rt95::allocs.load() == 0);
    CHECK (finite);
    CHECK (fx->latencySamples() > 0);
}

TEST (yw_tune_slot_and_project_round95)
{
    auto slot = yw::makeEffectSlot (yw::EffectKind::ywTune);
    CHECK (std::string (yw::effectInfo (yw::EffectKind::ywTune).name) == "YW Tune");
    CHECK ((int) yw::effectInfo (yw::EffectKind::ywTune).params.size() == yw::ywtune::count);
    CHECK (yw::formatEffectParam (yw::EffectKind::ywTune, yw::ywtune::scale, slot.params[yw::ywtune::scale], 120.0) == "Major");
    CHECK (yw::formatEffectParam (yw::EffectKind::ywTune, yw::ywtune::retune, slot.params[yw::ywtune::retune], 120.0) == "20 ms");
    CHECK (yw::formatEffectParam (yw::EffectKind::ywTune, yw::ywtune::reference, slot.params[yw::ywtune::reference], 120.0) == "440.0 Hz");
    CHECK (yw::formatEffectParam (yw::EffectKind::ywTune, yw::ywtune::range, slot.params[yw::ywtune::range], 120.0) == "Alto / Tenor");
    CHECK (yw::formatEffectParam (yw::EffectKind::ywTune, yw::ywtune::transpose, 1.0f, 120.0) == "+12 st");
    CHECK (yw::ywtune::latencyFor (44100.0, 0, true) < yw::ywtune::latencyFor (44100.0, 3, false));
}

namespace r95
{
// a Newtone project (our reading of the XML, REA_rodada73 §2 / REA_rodada95 §2) with three notes of `len` samples
std::string newtoneXml (int len, const std::string& note1Extra, const std::string& global = "")
{
    std::string x = "<NewToneProject NumOfTracks=\"1\"><Tracks><Trk_0 TrackNum=\"0\" Type=\"class CzplJPositionablePSOLASource\" FilePath=\"v.wav\">"
                    "<AudioObject SelfBPM=\"120.0\" OriginalFileLength=\"" + std::to_string (3 * len) + "\" " + global + ">";
    for (int k = 0; k < 3; ++k)
    {
        const auto a = std::to_string (k * len), b = std::to_string ((k + 1) * len);
        x += "<Pitch_" + std::to_string (k) + " " + (k == 1 ? note1Extra : std::string()) + " fOrigPitch=\"60.0\" fCurPitch=\"60.0\" iOrigStartSamplePos=\"" + a + "\" iOrigEndSamplePos=\"" + b
             + "\" iCurStartSamplePos=\"" + a + "\" iCurEndSamplePos=\"" + b + "\" fPitchSmoothInTime=\"0.0\" fPitchSmoothOutTime=\"0.0\"/>";
    }
    return x + "</AudioObject></Trk_0></Tracks></NewToneProject>";
}

yw::newtonefx::Track newtoneTrack (int len, const std::string& note1Extra, const std::string& global = "")
{
    const auto s = yw::newtonefx::parse (newtonetest::state (0, 4, newtoneXml (len, note1Extra, global), ""));
    return s && ! s->tracks.empty() ? s->tracks.front() : yw::newtonefx::Track {};
}
} // namespace r95

TEST (newtone_engine_edits_round95)
{
    // REA_rodada95 §2 [EXEC] → YW's engine: the shift is fCurrentPitchOffset + fAdditionalPitchOffset (fCurPitch is not
    // read), unedited notes are the source sample for sample, a disabled note is silent, fCurVolume is a plain gain
    const double fs = 44100.0;
    const int len = 22050;
    yw::SampleData src;
    src.sampleRate = fs;
    src.left = r95::vowel (fs, 3 * len, [] (double t) { return t < 0.5 ? 220.0 : t < 1.0 ? 246.94 : 261.63; });

    CHECK (yw::newtonefx::renderEdits (src, r95::newtoneTrack (len, "fCurPitch=\"62.0\"")) == nullptr); // display only
    const auto track = r95::newtoneTrack (len, "fCurrentPitchOffset=\"1.5\" fAdditionalPitchOffset=\"0.5\"");
    CHECK (track.pitches.size() == 3 && track.pitches[1].shift == 2.0 && track.pitches[1].edited (1.0) && ! track.pitches[0].edited (1.0));
    const auto up = yw::newtonefx::renderEdits (src, track);
    CHECK (up != nullptr && (int) up->left.size() == 3 * len && up->right.empty());
    if (up != nullptr)
    {
        CHECK (std::equal (src.left.begin(), src.left.begin() + len, up->left.begin()));
        CHECK (std::equal (src.left.begin() + 2 * len, src.left.end(), up->left.begin() + 2 * len));
        CHECK (std::abs (r95::cents (r95::measureHz (up->left, len + 6000, 4096, fs), 246.94 * std::exp2 (2.0 / 12.0))) < 3.0);
        // formants stay (TD-PSOLA)
        CHECK (std::abs (r95::formantCentre (up->left, len + 6000, 4096, fs) / r95::formantCentre (src.left, len + 6000, 4096, fs) - 1.0) < 0.07);
    }

    const auto quiet = yw::newtonefx::renderEdits (src, r95::newtoneTrack (len, "fCurVolume=\"0.5\""));
    const auto off = yw::newtonefx::renderEdits (src, r95::newtoneTrack (len, "bIsEnabled=\"0\""));
    const auto formant = yw::newtonefx::renderEdits (src, r95::newtoneTrack (len, "fCurFormantFactor=\"1.2\""));
    const auto global = yw::newtonefx::renderEdits (src, r95::newtoneTrack (len, "", "GlobalFormantShift=\"0.8\""));
    CHECK (quiet != nullptr && off != nullptr && formant != nullptr && global != nullptr);
    if (quiet != nullptr && off != nullptr && formant != nullptr && global != nullptr)
    {
        double a = 0.0, b = 0.0;
        for (int i = len + 2000; i < 2 * len - 2000; ++i)
        {
            a += (double) src.left[(std::size_t) i] * src.left[(std::size_t) i];
            b += (double) quiet->left[(std::size_t) i] * quiet->left[(std::size_t) i];
        }
        CHECK (std::abs (10.0 * std::log10 (b / a) + 6.02) < 0.3);
        CHECK (std::all_of (off->left.begin() + len, off->left.begin() + 2 * len, [] (float v) { return v == 0.0f; }));
        CHECK (r95::formantCentre (formant->left, len + 6000, 4096, fs) > 1.08 * r95::formantCentre (src.left, len + 6000, 4096, fs));
        CHECK (r95::formantCentre (global->left, len + 6000, 4096, fs) < 0.92 * r95::formantCentre (src.left, len + 6000, 4096, fs));
        CHECK (std::abs (r95::cents (r95::measureHz (formant->left, len + 6000, 4096, fs), 246.94)) < 3.0);
    }

    // timing: the note plays 25 % longer (source read at 0.8 per output sample), pitch kept; it cuts the next one's head
    const auto longer = yw::newtonefx::renderEdits (src, r95::newtoneTrack (len, "fCurInvStretchFactor=\"0.8\" iCurEndSamplePos=\"" + std::to_string (len + len * 5 / 4) + "\""));
    CHECK (longer != nullptr && longer != nullptr && std::abs (r95::cents (r95::measureHz (longer->left, 2 * len + 2000, 2048, fs), 246.94)) < 5.0);

    // a pitch step into the next note ramps over that note's smoothing time (§2.4)
    const auto ramp = yw::newtonefx::renderEdits (src, r95::newtoneTrack (len, "fCurrentPitchOffset=\"-2.0\" fPitchSmoothOutTime=\"0.0\""));
    CHECK (ramp != nullptr);

    // the effect plays the rendered edits in slave mode
    auto sample = std::make_shared<yw::SampleData> (src);
    auto fx = yw::newtonefx::make (sample, newtonetest::state (0, 4, r95::newtoneXml (len, "fCurrentPitchOffset=\"2.0\""), ""));
    fx->prepare (fs);
    yw::EffectParams on {};
    on[0] = 1.0f;
    std::vector<float> l (3 * len, 0.0f), r (3 * len, 0.0f);
    const auto tps = yw::ticksPerSample (120.0, fs);
    fx->setPlaying (true);
    for (int i = 0; i < 3 * len; i += 512)
    {
        fx->setSongPosition (i * tps);
        fx->process (l.data() + i, r.data() + i, std::min (512, 3 * len - i), on, 120.0);
    }
    CHECK (std::abs (r95::cents (r95::measureHz (l, len + 6000, 4096, fs), 246.94 * std::exp2 (2.0 / 12.0))) < 3.0);
    CHECK (std::equal (src.left.begin(), src.left.begin() + len, l.begin()));
}

TEST (yw_tune_editor_layout_round95)
{
    // YW Tune's own page in the plugin-window layout system: every parameter has a control inside the body
    const auto& l = yw::plugui::layoutFor (yw::EffectKind::ywTune);
    std::vector<bool> placed ((std::size_t) yw::ywtune::count, false);
    bool inside = true;
    for (const auto& item : l.items)
    {
        inside = inside && item.x >= 0 && item.y >= 0 && item.x + item.w <= (float) l.width + 0.5f && item.y + item.h <= (float) l.height + 0.5f;
        if (yw::plugui::isValueControl (item.type) && (item.style & yw::plugui::pageTabs) == 0) // (round 128: page tabs)
        {
            const auto p = yw::plugui::paramIndex (yw::EffectKind::ywTune, item.param);
            CHECK (p >= 0);
            if (p >= 0)
                placed[(std::size_t) p] = true;
        }
        // round 128: the keyboard display edits the note switches (on / remove and bypass); the Scale menu also
        // lists the extra scales (More scales)
        if (item.type == yw::plugui::Item::display && item.display != nullptr && std::string (item.display) == "tunekeys")
            for (int k = 0; k < 12; ++k)
            {
                placed[(std::size_t) (yw::ywtune::noteC + k)] = placed[(std::size_t) (yw::ywtune::bypassC + k)] = true;
                placed[(std::size_t) (yw::ywtune::noteDirC + k)] = true; // round 147b: the note directions
                placed[(std::size_t) yw::ywtune::rangeLow] = placed[(std::size_t) yw::ywtune::rangeHigh] = true; // and the range bar
                placed[(std::size_t) yw::ywtune::receiveScale] = true; // round 150b: the strip's Receive switch
                placed[(std::size_t) yw::ywtune::groupOctaves] = true; // round 152b: Group octaves and the key map
                for (int m = 0; m < yw::ywtune::keyMapParams; ++m) placed[(std::size_t) (yw::ywtune::keyMap1 + m)] = true;
            }
        if (item.type == yw::plugui::Item::menu && item.param != nullptr && std::string (item.param) == "Scale")
            placed[(std::size_t) yw::ywtune::moreScales] = true;
    }
    CHECK (inside);
    CHECK (std::all_of (placed.begin(), placed.end(), [] (bool b) { return b; }));
}

TEST (yw_tune_substitutes_auto_tune_round95)
{
    // a missing Auto-Tune Pro (FL wrapper record, VST3 class ID from real projects) becomes YW Tune, keeping the original
    using namespace substtest;
    yw::ForeignPlugin at;
    at.format = yw::ForeignPlugin::Format::vst3;
    at.classId = classIdOf ("415453563854413175746f2d54756e65");
    at.hasClassId = true;
    at.name = "Auto-Tune Pro";
    at.state = flVst3State (juceXmlComponent ("<AutoTune/>"));
    const auto id = yw::pluginIdentity (at);
    const auto* entry = yw::findPluginSubstitute (id, false);
    CHECK (entry != nullptr && entry->key == "antares-auto-tune" && entry->effect == yw::EffectKind::ywTune);
    yw::EffectSlot slot;
    slot.kind = yw::EffectKind::plugin;
    slot.plugin.name = at.name;
    slot.plugin.foreign = at;
    CHECK (yw::substituteEffectSlot (slot, id));
    CHECK (slot.kind == yw::EffectKind::ywTune && slot.isSubstitute() && slot.mix == 1.0f);
    // its key is not in this state: a gentle chromatic correction (round 113), not C major
    CHECK (yw::vocal::Scale (yw::ywtune::scaleIndex (slot.params[yw::ywtune::scale])) == yw::vocal::Scale::chromatic);

    // by name too (another edition)
    yw::ForeignPlugin artist;
    artist.format = yw::ForeignPlugin::Format::vst3;
    artist.name = "Auto-Tune Artist";
    const auto* byName = yw::findPluginSubstitute (yw::pluginIdentity (artist), false);
    CHECK (byName != nullptr && byName->effect == yw::EffectKind::ywTune);
}
