/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ActivateFonts.h"

#include <Ecore_File.h>
#include <fontconfig/fontconfig.h>
#include <string>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WTR {

static Vector<String> getFontDirectories()
{
    Vector<String> fontDirPaths;

#if ENABLE(TIZEN_WEBKIT2_EFL_WTR)
    fontDirPaths.append(String("/usr/share/fonts/"));
#endif
    fontDirPaths.append(String("/usr/share/fonts/TTF/"));
    fontDirPaths.append(String("/usr/share/fonts/truetype/ttf-liberation/"));
    fontDirPaths.append(String("/usr/share/fonts/liberation/"));
    fontDirPaths.append(String("/usr/share/fonts/truetype/ttf-dejavu/"));
    fontDirPaths.append(String("/usr/share/fonts/dejavu/"));
    fontDirPaths.append(String("/usr/share/fonts/OTF/")); // MathML
    fontDirPaths.append(String("/usr/share/fonts/opentype/stix/")); // MathML
    fontDirPaths.append(String("/usr/share/fonts/stix/")); // MathML

    return fontDirPaths;
}

static Vector<String> getFontFiles()
{
    Vector<String> fontFilePaths;

    const char* fontConfDir = getenv("TEST_RUNNER_FONT_CONF_PATH");

    std::string ahemFontPath(fontConfDir);
    ahemFontPath += "/AHEM____.TTF";
    // Ahem is used by many layout tests.
    fontFilePaths.append(String(ahemFontPath.data()));

    std::string format(fontConfDir);
    format += "/../../fonts/WebKitWeightWatcher%i00.ttf";

    for (int i = 1; i <= 9; i++) {
        char fontPath[PATH_MAX];
        snprintf(fontPath, PATH_MAX - 1, format.data(), i);
        fontFilePaths.append(String(fontPath));
    }

    return fontFilePaths;
}

static size_t addFontDirectories(const Vector<String>& fontDirectories, FcConfig* config)
{
    size_t addedDirectories = 0;

    for (Vector<String>::const_iterator it = fontDirectories.begin(); it != fontDirectories.end(); ++it) {
        const CString currentDirectory = (*it).utf8();
        const char* path = currentDirectory.data();

        if (ecore_file_is_dir(path)) {
            if (!FcConfigAppFontAddDir(config, reinterpret_cast<const FcChar8*>(path))) {
                fprintf(stderr, "Could not load font at %s!\n", path);
                continue;
            }

            ++addedDirectories;
        }
    }

    return addedDirectories;
}

static void addFontFiles(const Vector<String>& fontFiles, FcConfig* config)
{
    for (Vector<String>::const_iterator it = fontFiles.begin(); it != fontFiles.end(); ++it) {
        const CString currentFile = (*it).utf8();
        const char* path = currentFile.data();

        if (!FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8*>(path)))
            fprintf(stderr, "Could not load font at %s!\n", path);
    }
}

void addFontsToEnvironment()
{
    FcInit();

    // Load our configuration file, which sets up proper aliases for family
    // names like sans, serif and monospace.
    FcConfig* config = FcConfigCreate();
    std::string fontConfigFilename(getenv("TEST_RUNNER_FONT_CONF_PATH"));
    fontConfigFilename += "/fonts.conf";

    if (!FcConfigParseAndLoad(config, reinterpret_cast<const FcChar8*>(fontConfigFilename.data()), true)) {
        fprintf(stderr, "Couldn't load font configuration file from: %s\n", fontConfigFilename.data());
        exit(1);
    }

    if (!addFontDirectories(getFontDirectories(), config)) {
        fprintf(stderr, "None of the font directories could be added. Either install them "
                        "or file a bug at http://bugs.webkit.org if they are installed in "
                        "another location.\n");
        exit(1);
    }

    addFontFiles(getFontFiles(), config);

    if (!FcConfigSetCurrent(config)) {
        fprintf(stderr, "Could not set the current font configuration!\n");
        exit(1);
    }
}

void activateFonts()
{
    addFontsToEnvironment();
}

} // namespace WTR
