#pragma once
// GameAudio.h  —  irrKlang audio for STELLAR EXPEDITION
//
// Uses irrKlang (https://github.com/jonas2602/irrKlang).
// SDK lives in: irrklang/include/   irrklang/lib/   irrklang/bin/
// The vcxproj links irrKlang.lib and adds the include path.

#if __has_include(<irrKlang.h>)
#  define HAS_IRRKLANG 1
#  include <irrKlang.h>
#  pragma comment(lib, "irrKlang.lib")
using namespace irrklang;
#else
#  define HAS_IRRKLANG 0
#endif

#include <string>
#include <cmath>

class GameAudio
{
public:
    // Volume controls (0..1) — read/written by settings UI
    float musicVol = 0.75f;
    float sfxVol   = 1.00f;

    void init()
    {
#if HAS_IRRKLANG
        engine = createIrrKlangDevice();
        if (!engine) { printf("[Audio] irrKlang: createIrrKlangDevice failed\n"); return; }

        // Pre-start looping tracks at volume 0 so we can fade them in/out cleanly.
        // Menu music — looping, starts playing immediately (paused via volume=0 elsewhere)
        sndMenu = engine->play2D("media/audio/menu_music.wav",    /*loop*/true, /*startPaused*/true, /*track*/true);
        sndAmb  = engine->play2D("media/audio/space_ambient.ogg", true, true, true);

        if (sndMenu) { sndMenu->setVolume(0.0f); sndMenu->grab(); }
        if (sndAmb)  { sndAmb->setVolume(0.0f);  sndAmb->grab();  }

        // Rumble — looping, always running, volume set each frame in update()
        sndRumble = engine->play2D("media/audio/bh_rumble.ogg", true, false, true);
        if (sndRumble) { sndRumble->setVolume(0.0f); sndRumble->grab(); }

        ready = true;
        printf("[Audio] irrKlang ready\n");
#endif
    }

    void playMenuMusic()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        if (sndMenu) {
            sndMenu->setIsPaused(false);
            sndMenu->setVolume(musicVol * 0.70f);
        }
        // Silence ambient while on menu
        if (sndAmb) { sndAmb->setVolume(0.0f); sndAmb->setIsPaused(true); }
#endif
    }

    void stopMenuMusic()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        if (sndMenu) { sndMenu->setIsPaused(true); sndMenu->setVolume(0.0f); }
        // Start game ambient at 30% of music volume
        if (sndAmb) {
            sndAmb->setVolume(0.30f * musicVol);
            sndAmb->setIsPaused(false);
        }
#endif
    }

    void applyVolumes()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        if (sndMenu && !sndMenu->getIsPaused())
            sndMenu->setVolume(musicVol * 0.70f);
        if (sndAmb && !sndAmb->getIsPaused())
            sndAmb->setVolume(0.30f * musicVol);
#endif
    }

    // Call every frame — dangerLevel 0..1, inFall true during BH cinematic
    void update(float dangerLevel, bool inFall)
    {
#if HAS_IRRKLANG
        if (!ready) return;

        // BH rumble volume
        float rumVol = dangerLevel * dangerLevel * 0.80f * sfxVol;
        if (inFall) rumVol = 0.90f * sfxVol;
        if (sndRumble) sndRumble->setVolume(rumVol);

        // Fade ambient during fall
        if (sndAmb) {
            float target = inFall ? 0.0f : 0.30f * musicVol;
            float cur = sndAmb->getVolume();
            float step = inFall ? 0.005f : 0.02f;
            float next = cur + (target - cur) * step;
            sndAmb->setVolume(std::fmax(0.0f, next));
        }
#else
        (void)dangerLevel; (void)inFall;
#endif
    }

    // ---- one-shot SFX ----
    void playCellCollect()  { playOnce("media/audio/cell_collect.wav",  0.70f * sfxVol); }
    void playDangerAlarm()  { playOnce("media/audio/danger_alarm.wav",  0.55f * sfxVol); }
    void playTetherStress() { playOnce("media/audio/tether_stress.wav", 0.40f * sfxVol); }
    void playEventHorizon() { playOnce("media/audio/event_horizon.wav", 0.85f * sfxVol); }
    void playWinFanfare()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        if (sndAmb) sndAmb->setVolume(0.10f * musicVol);
        playOnce("media/audio/win_fanfare.wav", 0.85f * sfxVol);
#endif
    }

    // ---- ending music ----
    void playWinEnding()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        stopWinEnding();
        if (sndAmb)  { sndAmb->setVolume(0.0f);  sndAmb->setIsPaused(true); }
        if (sndMenu) { sndMenu->setVolume(0.0f);  sndMenu->setIsPaused(true); }
        sndWinEnd = engine->play2D("media/audio/win_ending.mp3", /*loop*/true, false, true);
        if (sndWinEnd) { sndWinEnd->setVolume(musicVol * 0.90f); sndWinEnd->grab(); }
#endif
    }

    void stopWinEnding()
    {
#if HAS_IRRKLANG
        if (!sndWinEnd) return;
        sndWinEnd->stop(); sndWinEnd->drop(); sndWinEnd = nullptr;
#endif
    }

    void playLoseEnding()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        stopLoseEnding();
        if (sndAmb)  { sndAmb->setVolume(0.0f);  sndAmb->setIsPaused(true); }
        if (sndMenu) { sndMenu->setVolume(0.0f);  sndMenu->setIsPaused(true); }
        sndLoseEnd = engine->play2D("media/audio/lose_ending.mp3", /*loop*/true, false, true);
        if (sndLoseEnd) { sndLoseEnd->setVolume(musicVol * 0.90f); sndLoseEnd->grab(); }
#endif
    }

    void stopLoseEnding()
    {
#if HAS_IRRKLANG
        if (!sndLoseEnd) return;
        sndLoseEnd->stop(); sndLoseEnd->drop(); sndLoseEnd = nullptr;
#endif
    }

    // ---- boost looping ----
    void startBoost()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        if (!sndBoost || sndBoost->isFinished()) {
            if (sndBoost) { sndBoost->drop(); sndBoost = nullptr; }
            sndBoost = engine->play2D("media/audio/boost_whoosh.wav", true, false, true);
            if (sndBoost) { sndBoost->setVolume(0.55f * sfxVol); sndBoost->grab(); }
        }
#endif
    }

    void stopBoost()
    {
#if HAS_IRRKLANG
        if (!ready || !sndBoost) return;
        sndBoost->stop();
        sndBoost->drop();
        sndBoost = nullptr;
#endif
    }

    // ---- BH fall cinematic ----
    void startBHFall()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        if (sndAmb) sndAmb->setVolume(0.0f);
        playOnce("media/audio/bh_fall_descent.ogg", 0.80f * sfxVol);
#endif
    }

    // ---- stop all in-game sounds when returning to main menu ----------------
    // Covers every looping/persistent sound that the game can start:
    //   win/lose ending music, boost whoosh, void ambient, BH rumble.
    // The ambient track and menu music are handled separately by playMenuMusic().
    void stopAllGameSounds()
    {
        stopWinEnding();
        stopLoseEnding();
        stopBoost();
        stopVoidAmbient();
#if HAS_IRRKLANG
        // BH rumble is always looping — zero its volume explicitly because
        // audio.update() does not run while showMenu==true (early return),
        // so the volume would otherwise stay frozen at whatever danger level
        // the player had when they left the game.
        if (sndRumble) sndRumble->setVolume(0.0f);
#endif
    }

    // ---- void ambient (post-death screen) ----
    void startVoidAmbient()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        stopVoidAmbient();
        sndVoid = engine->play2D("media/audio/silence_void.ogg", true, false, true);
        if (sndVoid) { sndVoid->setVolume(0.55f * musicVol); sndVoid->grab(); }
#endif
    }

    void stopVoidAmbient()
    {
#if HAS_IRRKLANG
        if (!sndVoid) return;
        sndVoid->stop();
        sndVoid->drop();
        sndVoid = nullptr;
#endif
    }

    void shutdown()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        stopBoost();
        stopVoidAmbient();
        stopWinEnding();
        stopLoseEnding();
        auto safeRelease = [](ISound*& s) {
            if (s) { s->stop(); s->drop(); s = nullptr; }
        };
        safeRelease(sndMenu);
        safeRelease(sndAmb);
        safeRelease(sndRumble);
        engine->drop();
        engine = nullptr;
        ready = false;
#endif
    }

private:
#if HAS_IRRKLANG
    ISoundEngine* engine  = nullptr;
    bool          ready   = false;

    // Persistent looping sounds (grab()'d so engine doesn't auto-delete them)
    ISound* sndMenu    = nullptr;
    ISound* sndAmb     = nullptr;
    ISound* sndRumble  = nullptr;
    ISound* sndBoost   = nullptr;
    ISound* sndVoid    = nullptr;
    ISound* sndWinEnd  = nullptr;
    ISound* sndLoseEnd = nullptr;

    void playOnce(const char* path, float vol)
    {
        if (!engine) return;
        ISound* s = engine->play2D(path, false, false, false);
        if (s) s->setVolume(vol);
        // no grab — engine owns it and auto-deletes when finished
    }
#endif
};
