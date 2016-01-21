#ifndef ONEFFECT_H
#define ONEFFECT_H

#include "RenderableEffect.h"


class OnEffect : public RenderableEffect
{
    public:
        OnEffect(int id);
        virtual ~OnEffect();
        virtual bool CanBeRandom() {return false;}
    
        virtual void Render(Effect *effect, const SettingsMap &settings, RenderBuffer &buffer);
        virtual int DrawEffectBackground(const Effect *e, int x1, int y1, int x2, int y2);

    protected:
        virtual wxPanel *CreatePanel(wxWindow *parent);
    private:
};

#endif // ONEFFECT_H