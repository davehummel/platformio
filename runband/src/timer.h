#ifndef DH_TIMER_H__
#define DH_TIMER_H__

class TIMER
{

  public:
    void start(void)
    {
        startTime = millis();
    }

    void stop(void)
    {
        startTime = 0xFFFFFFFF;
    }

    void evalSeconds(void)
    {
        if (startTime = 0xFFFFFFFF)
        {
            return;
        }
        secondsPassed = (millis() - startTime) / 1000;
    }

    void evalProgressSound(void)
    {
        if (secondsPassed == 0)
        {
            // sounds.playback[0] = 1000;
            // sounds.playback[1] = 1000;
        }
    }

  private:
    uint32_t startTime = 0xFFFFFFFF;
    uint16_t secondsPassed = 0;
    uint32_t lastSndTime = 0;
};
#endif
