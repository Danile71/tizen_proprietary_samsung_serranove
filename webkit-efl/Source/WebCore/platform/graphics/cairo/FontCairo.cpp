/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Holger Hans Peter Freyther
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Font.h"

#include "AffineTransform.h"
#include "CairoUtilities.h"
#include "GlyphBuffer.h"
#include "Gradient.h"
#include "GraphicsContext.h"
#include "PlatformContextCairo.h"
#include "ImageBuffer.h"
#include "Pattern.h"
#include "ShadowBlur.h"
#include "SimpleFontData.h"

#if ENABLE(TIZEN_EMOJI_FONT)
#include "TizenColorUtilities.h"
#endif

namespace WebCore {

#if ENABLE(TIZEN_EMOJI_FONT)
static bool applyEmojiFontColor(GraphicsContext* graphicsContext, const HashMap<unsigned short, int>& emojiColorMap, GlyphBufferGlyph* glyphs, int numGlyphs)
{
    cairo_t* context = graphicsContext->platformContext()->cr();
    EmojiColors emojiColor = NoEmoji;

    bool bEmojiApplied = false;

    HashMap<unsigned short, int>::const_iterator it;

    for (int i = 0; i < numGlyphs; i++) {
        it = emojiColorMap.find(glyphs[i].index);

        if (it != emojiColorMap.end()) {
            if (graphicsContext->useEmojiColor())
                emojiColor = static_cast<EmojiColors>(it->second);
            ASSERT(emojiColor == NoEmoji);
            cairo_save(context);
            switch (emojiColor) {
                case EmojiBlack:
                    cairo_set_source_rgb(context, 0.43, 0.43, 0.43); // #6E6E6E Gray for black theme
                    break;
                case EmojiRed:
                    cairo_set_source_rgb(context, 1, 0, 0); // #FF0000 Red
                    break;
                case EmojiLime:
                    cairo_set_source_rgb(context, 0, 1, 0); // #00FF00 Lime
                    break;
                case EmojiBlue:
                    cairo_set_source_rgb(context, 0, 0, 1); // #0000FF Blue
                    break;
                case EmojiOrange:
                    cairo_set_source_rgb(context, 1, 0.6, 0); // #FF9900 Orange
                    break;
                case EmojiFuchsia:
                    cairo_set_source_rgb(context, 1, 0, 1); // #FF00FF Fuchsia
                    break;
                case EmojiMaroon:
                    cairo_set_source_rgb(context, 0.5, 0, 0); // #800000 Maroon
                    break;
                case EmojiNavy:
                    cairo_set_source_rgb(context, 0, 0, 0.5); // #000080 Navy
                    break;
                case EmojiPurple:
                    cairo_set_source_rgb(context, 0.5, 0, 0.5); // #800080 Purple
                    break;
                default:
                    ASSERT_NOT_REACHED();
                    break;
            }
            cairo_show_glyphs(context, &glyphs[i], 1);
            cairo_restore(context);
            bEmojiApplied = true;
        }
    }
    return bEmojiApplied;
}
#endif

#if ENABLE(TIZEN_EMOJI_FONT)
static void drawGlyphsToContext(GraphicsContext* graphicsContext, const SimpleFontData* font, GlyphBufferGlyph* glyphs, int numGlyphs)
#else
static void drawGlyphsToContext(cairo_t* context, const SimpleFontData* font, GlyphBufferGlyph* glyphs, int numGlyphs)
#endif
{
#if ENABLE(TIZEN_EMOJI_FONT)
    cairo_t* context = graphicsContext->platformContext()->cr();
#endif

    cairo_matrix_t originalTransform;
    float syntheticBoldOffset = font->syntheticBoldOffset();

#if ENABLE(TIZEN_EMOJI_FONT) && ENABLE(TIZEN_SYNTHETIC_BOLD_OFFSET)
    if (syntheticBoldOffset && !graphicsContext->getCTM().isIdentityOrTranslationOrFlipped()) {
        FloatSize horizontalUnitSizeInDevicePixels = graphicsContext->getCTM().mapSize(FloatSize(1, 0));
        float horizontalUnitLengthInDevicePixels = sqrtf(horizontalUnitSizeInDevicePixels.width() * horizontalUnitSizeInDevicePixels.width() + horizontalUnitSizeInDevicePixels.height() * horizontalUnitSizeInDevicePixels.height());
        if (horizontalUnitLengthInDevicePixels)
            syntheticBoldOffset /= horizontalUnitLengthInDevicePixels;
    }
#endif

    if (syntheticBoldOffset)
        cairo_get_matrix(context, &originalTransform);

    cairo_set_scaled_font(context, font->platformData().scaledFont());
#if ENABLE(TIZEN_EMOJI_FONT)
    if (!applyEmojiFontColor(graphicsContext, font->m_emojiColorMap, glyphs, numGlyphs))
#endif
    cairo_show_glyphs(context, glyphs, numGlyphs);

    if (syntheticBoldOffset) {
        cairo_translate(context, syntheticBoldOffset, 0);
#if ENABLE(TIZEN_EMOJI_FONT)
        if (!applyEmojiFontColor(graphicsContext, font->m_emojiColorMap, glyphs, numGlyphs))
#endif
        cairo_show_glyphs(context, glyphs, numGlyphs);
    }

    if (syntheticBoldOffset)
        cairo_set_matrix(context, &originalTransform);
}

static void drawGlyphsShadow(GraphicsContext* graphicsContext, const FloatPoint& point, const SimpleFontData* font, GlyphBufferGlyph* glyphs, int numGlyphs)
{
    ShadowBlur& shadow = graphicsContext->platformContext()->shadowBlur();

    if (!(graphicsContext->textDrawingMode() & TextModeFill) || shadow.type() == ShadowBlur::NoShadow)
        return;

    if (!shadow.mustUseShadowBlur(graphicsContext)) {
        // Optimize non-blurry shadows, by just drawing text without the ShadowBlur.
        cairo_t* context = graphicsContext->platformContext()->cr();
        cairo_save(context);

        FloatSize shadowOffset(graphicsContext->state().shadowOffset);
        cairo_translate(context, shadowOffset.width(), shadowOffset.height());
        setSourceRGBAFromColor(context, graphicsContext->state().shadowColor);
#if ENABLE(TIZEN_EMOJI_FONT)
        drawGlyphsToContext(graphicsContext, font, glyphs, numGlyphs);
#else
        drawGlyphsToContext(context, font, glyphs, numGlyphs);
#endif

        cairo_restore(context);
        return;
    }

    cairo_text_extents_t extents;
    cairo_scaled_font_glyph_extents(font->platformData().scaledFont(), glyphs, numGlyphs, &extents);
    FloatRect fontExtentsRect(point.x() + extents.x_bearing, point.y() + extents.y_bearing, extents.width, extents.height);

    if (GraphicsContext* shadowContext = shadow.beginShadowLayer(graphicsContext, fontExtentsRect)) {
#if ENABLE(TIZEN_EMOJI_FONT)
        drawGlyphsToContext(shadowContext, font, glyphs, numGlyphs);
#else
        drawGlyphsToContext(shadowContext->platformContext()->cr(), font, glyphs, numGlyphs);
#endif
        shadow.endShadowLayer(graphicsContext);
    }
}

void Font::drawGlyphs(GraphicsContext* context, const SimpleFontData* font, const GlyphBuffer& glyphBuffer,
                      int from, int numGlyphs, const FloatPoint& point) const
{
    if (!font->platformData().size())
        return;

    GlyphBufferGlyph* glyphs = const_cast<GlyphBufferGlyph*>(glyphBuffer.glyphs(from));

    float offset = point.x();
    for (int i = 0; i < numGlyphs; i++) {
        glyphs[i].x = offset;
        glyphs[i].y = point.y();
        offset += glyphBuffer.advanceAt(from + i);
    }

    PlatformContextCairo* platformContext = context->platformContext();
    drawGlyphsShadow(context, point, font, glyphs, numGlyphs);

    cairo_t* cr = platformContext->cr();
    cairo_save(cr);

    if (context->textDrawingMode() & TextModeFill) {
        platformContext->prepareForFilling(context->state(), PlatformContextCairo::AdjustPatternForGlobalAlpha);
#if ENABLE(TIZEN_EMOJI_FONT)
        drawGlyphsToContext(context, font, glyphs, numGlyphs);
#else
        drawGlyphsToContext(cr, font, glyphs, numGlyphs);
#endif
    }

    // Prevent running into a long computation within cairo. If the stroke width is
    // twice the size of the width of the text we will not ask cairo to stroke
    // the text as even one single stroke would cover the full wdth of the text.
    //  See https://bugs.webkit.org/show_bug.cgi?id=33759.
    if (context->textDrawingMode() & TextModeStroke && context->strokeThickness() < 2 * offset) {
        platformContext->prepareForStroking(context->state());
        cairo_set_line_width(cr, context->strokeThickness());

        // This may disturb the CTM, but we are going to call cairo_restore soon after.
        cairo_set_scaled_font(cr, font->platformData().scaledFont());
        cairo_glyph_path(cr, glyphs, numGlyphs);
        cairo_stroke(cr);
    }

    cairo_restore(cr);
}

}
