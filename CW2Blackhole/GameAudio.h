#pragma once
// GameAudio.h — wraps irrKlang for all in-game sounds and music

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
    // Volume sliders (0..1) — adjusted in settings menu
    float musicVol = 0.75f;
    float sfxVol   = 1.00f;

    void init()
    {
#if HAS_IRRKLANG
        engine = createIrrKlangDevice();
        if (!engine) { printf("[Audio] irrKlang: createIrrKlangDevice failed\n"); return; }

        // Pre-load looping tracks at volume 0 so they can be faded in/out cleanly
        sndMenu = engine->play2D("media/audio/menu_music.wav",    /*loop*/true, /*startPaused*/true, /*track*/true);
        sndAmb  = engine->play2D("media/audio/space_ambient.ogg", true, true, true);

        if (sndMenu) { sndMenu->setVolume(0.0f); sndMenu->grab(); }
        if (sndAmb)  { sndAmb->setVolume(0.0f);  sndAmb->grab();  }

        // Rumble always runs — volume is set each frame based on danger level
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
            // Start space ambient at 30% volume when entering the game
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

    // Call every frame to update the rumble volume and fade the ambient
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

    // One-shot sound effects
    void playCellCollect()  { playOnce("media/audio/cell_collect.wav",  0.70f * sfxVol); }
    void playDangerAlarm()  { playOnce("media/audio/danger_alarm.wav",  0.28f * sfxVol); }
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

    // Looping ending music (win or lose)
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

    // Boost whoosh — loops while shift is held
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

    // Black hole fall sound
    void startBHFall()
    {
#if HAS_IRRKLANG
        if (!ready) return;
        if (sndAmb) sndAmb->setVolume(0.0f);
        playOnce("media/audio/bh_fall_descent.ogg", 0.80f * sfxVol);
#endif
    }

    // Stop all in-game sounds when returning to the main menu
    void stopAllGameSounds()
    {
        stopWinEnding();
        stopLoseEnding();
        stopBoost();
        stopVoidAmbient();
#if HAS_IRRKLANG
        // Silence the rumble — it won't auto-reset because update() doesn't run on the menu
        if (sndRumble) sndRumble->setVolume(0.0f);
#endif
    }

    // Void ambient — plays on the post-death screen
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

    // Looping sounds — grabbed so the engine doesn't auto-delete them
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
        // no grab — engine deletes it automatically when done
    }
#endif
};
