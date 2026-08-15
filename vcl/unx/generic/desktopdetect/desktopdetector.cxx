/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <rtl/bootstrap.hxx>
#include <rtl/process.h>
#include <osl/thread.h>
#include <vcl/DesktopType.hxx>

#include <vclpluginapi.h>

#include <string.h>
#include <comphelper/string.hxx>

static bool is_plasma5_desktop()
{
    static const char* pFullVersion = getenv("KDE_FULL_SESSION");
    static const char* pSessionVersion = getenv("KDE_SESSION_VERSION");
    return pFullVersion && pSessionVersion && (0 == strcmp(pSessionVersion, "5"));
}

static bool is_plasma6_desktop()
{
    static const char* pFullVersion = getenv("KDE_FULL_SESSION");
    static const char* pSessionVersion = getenv("KDE_SESSION_VERSION");
    return pFullVersion && pSessionVersion && (0 == strcmp(pSessionVersion, "6"));
}

DesktopType get_desktop_environment()
{
    static const char *pOverride = getenv( "OOO_FORCE_DESKTOP" );

    if ( pOverride && *pOverride )
    {
        OString aOver( pOverride );

        if ( aOver.equalsIgnoreAsciiCase( "lxqt" ) )
            return DesktopType::LXQt;
        if (aOver.equalsIgnoreAsciiCase("plasma5") || aOver.equalsIgnoreAsciiCase("plasma"))
            return DesktopType::Plasma5;
        if (aOver.equalsIgnoreAsciiCase("plasma6"))
            return DesktopType::Plasma6;
        if ( aOver.equalsIgnoreAsciiCase( "gnome" ) )
            return DesktopType::GNOME;
        if ( aOver.equalsIgnoreAsciiCase( "gnome-wayland" ) )
            return DesktopType::GNOME;
        if ( aOver.equalsIgnoreAsciiCase( "unity" ) )
            return DesktopType::Unity;
        if ( aOver.equalsIgnoreAsciiCase( "xfce" ) )
            return DesktopType::Xfce;
        if ( aOver.equalsIgnoreAsciiCase( "mate" ) )
            return DesktopType::MATE;
        if ( aOver.equalsIgnoreAsciiCase( "none" ) )
            return DesktopType::Unknown;
    }

    OUString plugin;
    rtl::Bootstrap::get(u"SAL_USE_VCLPLUGIN"_ustr, plugin);

    if (plugin == "svp")
        return DesktopType::Headless;

    const char *pDesktop = getenv( "XDG_CURRENT_DESKTOP" );
    if ( pDesktop )
    {
        OString aCurrentDesktop( pDesktop, strlen( pDesktop ) );

        //it may be separated by colon ( e.g. unity:unity7:ubuntu )
        std::vector<OUString> aSplitCurrentDesktop = comphelper::string::split(
                OStringToOUString( aCurrentDesktop, RTL_TEXTENCODING_UTF8), ':');
        for (const auto& rCurrentDesktopStr : aSplitCurrentDesktop)
        {
            if ( rCurrentDesktopStr.equalsIgnoreAsciiCase( "unity" ) )
                return DesktopType::Unity;
            else if ( rCurrentDesktopStr.equalsIgnoreAsciiCase( "gnome" ) )
                return DesktopType::GNOME;
            else if ( rCurrentDesktopStr.equalsIgnoreAsciiCase( "lxqt" ) )
                return DesktopType::LXQt;
        }
    }

    const char *pSession = getenv( "DESKTOP_SESSION" );
    OString aDesktopSession;
    if ( pSession )
        aDesktopSession = OString( pSession, strlen( pSession ) );

    // fast environment variable checks
    if ( aDesktopSession.equalsIgnoreAsciiCase( "gnome" ) )
        return DesktopType::GNOME;
    else if ( aDesktopSession.equalsIgnoreAsciiCase( "gnome-wayland" ) )
        return DesktopType::GNOME;
    else if ( aDesktopSession.equalsIgnoreAsciiCase( "mate" ) )
        return DesktopType::MATE;
    else if ( aDesktopSession.equalsIgnoreAsciiCase( "xfce" ) )
        return DesktopType::Xfce;
    else if ( aDesktopSession.equalsIgnoreAsciiCase( "lxqt" ) )
        return DesktopType::LXQt;

    if (is_plasma5_desktop())
        return DesktopType::Plasma5;
    if (is_plasma6_desktop())
        return DesktopType::Plasma6;

    // tdf#121275 if we still can't tell, and WAYLAND_DISPLAY
    // is set, default to gtk3
    const char* pWaylandStr = getenv("WAYLAND_DISPLAY");
    if (pWaylandStr && *pWaylandStr)
        return DesktopType::GNOME;

    // these guys can be slower, with X property fetches,
    // round-trips etc. and so are done later.

    // get display to connect to
    const char* pDisplayStr = getenv( "DISPLAY" );

    int nParams = rtl_getAppCommandArgCount();
    OUString aParam;
    OString aBParm;
    for( int i = 0; i < nParams; i++ )
    {
        rtl_getAppCommandArg( i, &aParam.pData );
        if( i < nParams-1 && (aParam == "-display" || aParam == "--display" ) )
        {
            rtl_getAppCommandArg( i+1, &aParam.pData );
            aBParm = OUStringToOString( aParam, osl_getThreadTextEncoding() );
            pDisplayStr = aBParm.getStr();
            break;
        }
    }

    // no server at all
    if( ! pDisplayStr || !*pDisplayStr )
        return DesktopType::Headless;

    // warning: these checks are coincidental, GNOME does not
    // explicitly advertise itself
    if (getenv("GNOME_DESKTOP_SESSION_ID"))
        return DesktopType::GNOME;

    return DesktopType::Unknown;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
