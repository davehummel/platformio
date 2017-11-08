#ifndef DH_SOUNDS_H__
#define DH_SOUNDS_H__

#define SAMPLES 256

class Sounds
{

  public:
    Sounds(uint16_t pin)
    {
        sndPin = pin;
    }

    bool soundFree(void)
    {
        if (playbackPosition >= SAMPLES)
            return true;
        else
            return false;
    }

    void reset(void)
    {
        noTone(sndPin);
        playbackPosition = SAMPLES + 1;
        memset(playback, 0, sizeof(playback));
    }

    void play(void)
    {
        if (playbackPosition < SAMPLES)
        {
            return;
        }
        playbackPosition = 0;
    }

    void renderSound(void)
    {
        if (playbackPosition > SAMPLES)
        {
            return;
        }
        if (playbackPosition = SAMPLES)
        {
            noTone(sndPin);
        }
        else if ((playbackPosition == 0) || (playback[playbackPosition] != lastSnd))
        {
            tone(sndPin, lastSnd = playback[playbackPosition]);
            playback[playbackPosition] = 0;
        }
        playbackPosition++;
    }

    uint16_t playback[SAMPLES] = {0};

  private:
    uint8_t sndPin = 0;
    uint16_t lastSnd = 0;
    uint16_t playbackPosition = SAMPLES + 1;
};

#endif
