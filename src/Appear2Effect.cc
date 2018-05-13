/*
 * Copyright (C) 2018 Vlad Zagorodniy <vladzzag@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Appear2Effect.h"

// KConfigSkeleton
#include "appear2config.h"

// kwineffects
#include <kwinglutils.h>

// Qt
#include <QMatrix4x4>

Appear2Effect::Appear2Effect()
{
    initConfig<Appear2Config>();
    reconfigure(ReconfigureAll);

    connect(KWin::effects, &KWin::EffectsHandler::windowAdded,
        this, &Appear2Effect::start);
    connect(KWin::effects, &KWin::EffectsHandler::windowClosed,
        this, &Appear2Effect::stop);
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted,
        this, &Appear2Effect::stop);
    connect(KWin::effects, &KWin::EffectsHandler::windowMinimized,
        this, &Appear2Effect::stop);
}

Appear2Effect::~Appear2Effect()
{
}

void Appear2Effect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags);

    Appear2Config::self()->read();
    m_blacklist = Appear2Config::blacklist().toSet();
    m_duration = animationTime(Appear2Config::duration() > 0
            ? Appear2Config::duration()
            : 160);
    m_opacity = Appear2Config::opacity();
    m_pitch = Appear2Config::pitch();
    m_distance = Appear2Config::distance();
}

void Appear2Effect::prePaintScreen(KWin::ScreenPrePaintData& data, int time)
{
    auto it = m_animations.begin();
    while (it != m_animations.end()) {
        Timeline& t = *it;
        t.update(time);
        if (t.done()) {
            it = m_animations.erase(it);
        } else {
            ++it;
        }
    }

    if (!m_animations.isEmpty()) {
        data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;
    }

    KWin::effects->prePaintScreen(data, time);
}

void Appear2Effect::prePaintWindow(KWin::EffectWindow* w, KWin::WindowPrePaintData& data, int time)
{
    if (m_animations.contains(w)) {
        data.setTransformed();
    }

    KWin::effects->prePaintWindow(w, data, time);
}

void Appear2Effect::paintWindow(KWin::EffectWindow* w, int mask, QRegion region, KWin::WindowPaintData& data)
{
    auto it = m_animations.constFind(w);
    if (it != m_animations.cend()) {
        const qreal t = (*it).value();

        const QRectF screenGeo = KWin::GLRenderTarget::virtualScreenGeometry();
        const QRectF windowGeo = w->geometry();

        // Perspective projection distorts objects near edges
        // of the viewport. This is critical because distortions
        // near edges of the viewport are not desired with this effect.
        // To fix this, the center of the window will be moved to the origin,
        // after applying perspective projection, the center is moved back
        // to its "original" projected position. Overall, this is how the window
        // will be transformed:
        //  [move to the origin] -> [rotate] -> [translate] ->
        //    -> [perspective projection] -> [reverse "move to the origin"]
        const QMatrix4x4 oldProjMatrix = data.screenProjectionMatrix();
        const QVector3D invOffset = oldProjMatrix.map(QVector3D(windowGeo.center()));
        QMatrix4x4 invOffsetMatrix;
        invOffsetMatrix.translate(invOffset.x(), invOffset.y());
        data.setProjectionMatrix(invOffsetMatrix * oldProjMatrix);

        // Move the center of the window to the origin.
        const QPointF offset = screenGeo.center() - windowGeo.center();
        data.translate(offset.x(), offset.y());

        data.setRotationAxis(Qt::XAxis);
        data.setRotationAngle(interpolate(m_pitch, 0, t));
        data.setZTranslation(interpolate(m_distance, 0, t));
        data.multiplyOpacity(interpolate(m_opacity, 1, t));
    }

    KWin::effects->paintWindow(w, mask, region, data);
}

void Appear2Effect::postPaintScreen()
{
    if (!m_animations.isEmpty()) {
        KWin::effects->addRepaintFull();
    }

    KWin::effects->postPaintScreen();
}

bool Appear2Effect::isActive() const
{
    return !m_animations.isEmpty();
}

bool Appear2Effect::supported()
{
    return KWin::effects->isOpenGLCompositing()
        && KWin::effects->animationsSupported();
}

bool Appear2Effect::shouldAnimate(const KWin::EffectWindow* w) const
{
    if (KWin::effects->activeFullScreenEffect()) {
        return false;
    }

    const auto* addGrab = w->data(KWin::WindowAddedGrabRole).value<void*>();
    if (addGrab != nullptr && addGrab != this) {
        return false;
    }

    if (!w->isManaged()) {
        return false;
    }

    if (m_blacklist.contains(w->windowClass())) {
        return false;
    }

    return w->isNormalWindow()
        || w->isDialog();
}

void Appear2Effect::start(KWin::EffectWindow* w)
{
    if (!shouldAnimate(w)) {
        return;
    }

    // Tell other effects(like fade, for example) to ignore this window.
    w->setData(KWin::WindowAddedGrabRole, QVariant::fromValue(static_cast<void*>(this)));

    Timeline& t = m_animations[w];
    t.setDuration(m_duration);
    t.setEasingCurve(QEasingCurve::InCurve);
}

void Appear2Effect::stop(KWin::EffectWindow* w)
{
    m_animations.remove(w);
}
