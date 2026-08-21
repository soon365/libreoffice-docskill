/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#pragma once

#include <salframe.hxx>
#include <vcl/sysdata.hxx>

/** Win32 custom messages: merge VCL menubar into the DWM caption.
 *  Handled in WinSalFrame WndProc. Not on SalFrame (that header is in the VCL PCH).
 */
#if defined(_WIN32)
#define SAL_MSG_DOCSKILL_CAPTION_MENU (WM_USER + 170)
#define SAL_MSG_DOCSKILL_CAPTION_H    (WM_USER + 171)
#define SAL_MSG_DOCSKILL_CAPTION_BTN  (WM_USER + 172)
#define SAL_MSG_DOCSKILL_CAPTION_HIT  (WM_USER + 173)
#define SAL_MSG_DOCSKILL_SIDEBAR_TOGGLE (WM_USER + 174)
#define SAL_MSG_DOCSKILL_SIDEBAR_TOGGLE_PEEK (WM_USER + 175)
#define SAL_MSG_DOCSKILL_CAPTION_BTN_CLICK (WM_USER + 176)
#define SAL_MSG_DOCSKILL_TOGGLE_HIT (WM_USER + 177)
/** Deferred DwmExtendFrameIntoClientArea. Never extend the frame inline: DWM
 *  answers with WM_NCCALCSIZE / WM_SIZE, which re-enters VCL layout while the
 *  window tree is still being mutated (intermittent 0xC0000374 at startup). */
#define SAL_MSG_DOCSKILL_CAPTION_SYNC (WM_USER + 178)

inline void DocSkillSetCaptionMenu(SalFrame* pFrame, bool bOn)
{
    if (!pFrame)
        return;
    HWND hWnd = pFrame->GetSystemData().hWnd;
    if (!hWnd)
        return;
    // MUST Post, never Send: brdwin calls this during SetMenuBarWindow while
    // VCL still owns the window tree. SendMessage re-enters the frame WndProc
    // and nested SIZE/DwmExtend has caused STATUS_HEAP_CORRUPTION at startup.
    PostMessageW(hWnd, SAL_MSG_DOCSKILL_CAPTION_MENU, bOn ? 1 : 0, 0);
}

inline tools::Long DocSkillCaptionMenuHeight(SalFrame* pFrame)
{
    if (!pFrame)
        return 0;
    HWND hWnd = pFrame->GetSystemData().hWnd;
    if (!hWnd)
        return 0;
    return static_cast<tools::Long>(SendMessageW(hWnd, SAL_MSG_DOCSKILL_CAPTION_H, 0, 0));
}

inline tools::Long DocSkillCaptionButtonWidth(SalFrame* pFrame)
{
    if (!pFrame)
        return 0;
    HWND hWnd = pFrame->GetSystemData().hWnd;
    if (!hWnd)
        return 0;
    return static_cast<tools::Long>(SendMessageW(hWnd, SAL_MSG_DOCSKILL_CAPTION_BTN, 0, 0));
}

inline void DocSkillSetCaptionMenuHitRange(SalFrame* pFrame, tools::Long nLeft, tools::Long nRight)
{
    if (!pFrame)
        return;
    HWND hWnd = pFrame->GetSystemData().hWnd;
    if (hWnd)
        SendMessageW(hWnd, SAL_MSG_DOCSKILL_CAPTION_HIT, static_cast<WPARAM>(nLeft),
                     static_cast<LPARAM>(nRight));
}

inline void DocSkillSetCaptionToggleHitRange(SalFrame* pFrame, tools::Long nLeft, tools::Long nRight)
{
    if (!pFrame)
        return;
    HWND hWnd = pFrame->GetSystemData().hWnd;
    if (hWnd)
        SendMessageW(hWnd, SAL_MSG_DOCSKILL_TOGGLE_HIT, static_cast<WPARAM>(nLeft),
                     static_cast<LPARAM>(nRight));
}

inline void DocSkillPostSidebarToggle(SalFrame* pFrame)
{
    if (!pFrame)
        return;
    HWND hWnd = pFrame->GetSystemData().hWnd;
    if (hWnd)
        PostMessageW(hWnd, SAL_MSG_DOCSKILL_SIDEBAR_TOGGLE, 0, 0);
}

inline void DocSkillPostCaptionButton(SalFrame* pFrame, int nWhich)
{
    if (!pFrame || nWhich < 1 || nWhich > 3)
        return;
    HWND hWnd = pFrame->GetSystemData().hWnd;
    if (hWnd)
        PostMessageW(hWnd, SAL_MSG_DOCSKILL_CAPTION_BTN_CLICK, static_cast<WPARAM>(nWhich), 0);
}

inline bool DocSkillPeekSidebarToggle(SalFrame* pFrame)
{
    if (!pFrame)
        return false;
    HWND hWnd = pFrame->GetSystemData().hWnd;
    if (!hWnd)
        return false;
    return SendMessageW(hWnd, SAL_MSG_DOCSKILL_SIDEBAR_TOGGLE_PEEK, 0, 0) != 0;
}
#else
inline void DocSkillSetCaptionMenu(SalFrame*, bool) {}
inline tools::Long DocSkillCaptionMenuHeight(SalFrame*) { return 0; }
inline tools::Long DocSkillCaptionButtonWidth(SalFrame*) { return 0; }
inline void DocSkillSetCaptionMenuHitRange(SalFrame*, tools::Long, tools::Long) {}
inline void DocSkillSetCaptionToggleHitRange(SalFrame*, tools::Long, tools::Long) {}
inline void DocSkillPostSidebarToggle(SalFrame*) {}
inline void DocSkillPostCaptionButton(SalFrame*, int) {}
inline bool DocSkillPeekSidebarToggle(SalFrame*) { return false; }
#endif

