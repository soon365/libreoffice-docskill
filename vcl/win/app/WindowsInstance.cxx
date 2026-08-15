/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
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

#include <jobset.h>
#include <print.h>
#include <skia/win/gdiimpl.hxx>
#include <win/WindowsInstance.hxx>
#include <win/salprn.h>

#include <comphelper/solarmutex.hxx>
#include <o3tl/char16_t2wchar_t.hxx>
#include <o3tl/temporary.hxx>
#include <vcl/skia/SkiaHelper.hxx>

#include <winspool.h>
#if defined GetDefaultPrinter
#undef GetDefaultPrinter
#endif

static PrintQueueFlags ImplWinQueueStatusToSal(DWORD nWinStatus)
{
    PrintQueueFlags nStatus = PrintQueueFlags::NONE;
    if (nWinStatus & PRINTER_STATUS_PAUSED)
        nStatus |= PrintQueueFlags::Paused;
    if (nWinStatus & PRINTER_STATUS_ERROR)
        nStatus |= PrintQueueFlags::Error;
    if (nWinStatus & PRINTER_STATUS_PENDING_DELETION)
        nStatus |= PrintQueueFlags::PendingDeletion;
    if (nWinStatus & PRINTER_STATUS_PAPER_JAM)
        nStatus |= PrintQueueFlags::PaperJam;
    if (nWinStatus & PRINTER_STATUS_PAPER_OUT)
        nStatus |= PrintQueueFlags::PaperOut;
    if (nWinStatus & PRINTER_STATUS_MANUAL_FEED)
        nStatus |= PrintQueueFlags::ManualFeed;
    if (nWinStatus & PRINTER_STATUS_PAPER_PROBLEM)
        nStatus |= PrintQueueFlags::PaperProblem;
    if (nWinStatus & PRINTER_STATUS_OFFLINE)
        nStatus |= PrintQueueFlags::Offline;
    if (nWinStatus & PRINTER_STATUS_IO_ACTIVE)
        nStatus |= PrintQueueFlags::IOActive;
    if (nWinStatus & PRINTER_STATUS_BUSY)
        nStatus |= PrintQueueFlags::Busy;
    if (nWinStatus & PRINTER_STATUS_PRINTING)
        nStatus |= PrintQueueFlags::Printing;
    if (nWinStatus & PRINTER_STATUS_OUTPUT_BIN_FULL)
        nStatus |= PrintQueueFlags::OutputBinFull;
    if (nWinStatus & PRINTER_STATUS_WAITING)
        nStatus |= PrintQueueFlags::Waiting;
    if (nWinStatus & PRINTER_STATUS_PROCESSING)
        nStatus |= PrintQueueFlags::Processing;
    if (nWinStatus & PRINTER_STATUS_INITIALIZING)
        nStatus |= PrintQueueFlags::Initializing;
    if (nWinStatus & PRINTER_STATUS_WARMING_UP)
        nStatus |= PrintQueueFlags::WarmingUp;
    if (nWinStatus & PRINTER_STATUS_TONER_LOW)
        nStatus |= PrintQueueFlags::TonerLow;
    if (nWinStatus & PRINTER_STATUS_NO_TONER)
        nStatus |= PrintQueueFlags::NoToner;
    if (nWinStatus & PRINTER_STATUS_PAGE_PUNT)
        nStatus |= PrintQueueFlags::PagePunt;
    if (nWinStatus & PRINTER_STATUS_USER_INTERVENTION)
        nStatus |= PrintQueueFlags::UserIntervention;
    if (nWinStatus & PRINTER_STATUS_OUT_OF_MEMORY)
        nStatus |= PrintQueueFlags::OutOfMemory;
    if (nWinStatus & PRINTER_STATUS_DOOR_OPEN)
        nStatus |= PrintQueueFlags::DoorOpen;
    if (nWinStatus & PRINTER_STATUS_SERVER_UNKNOWN)
        nStatus |= PrintQueueFlags::StatusUnknown;
    if (nWinStatus & PRINTER_STATUS_POWER_SAVE)
        nStatus |= PrintQueueFlags::PowerSave;
    if (nStatus == PrintQueueFlags::NONE && !(nWinStatus & PRINTER_STATUS_NOT_AVAILABLE))
        nStatus |= PrintQueueFlags::Ready;
    return nStatus;
}

WindowsInstance::WindowsInstance(std::unique_ptr<comphelper::SolarMutex> pMutex, SalData* pSalData)
    : SalInstance(std::move(pMutex), pSalData)
{
    WinSkiaSalGraphicsImpl::prepareSkia();
}

WindowsInstance::~WindowsInstance() { SkiaHelper::cleanup(); }

SalInfoPrinter* WindowsInstance::CreateInfoPrinter(SalPrinterQueueInfo& rQueueInfo,
                                                   ImplJobSetup& rSetupData)
{
    WinSalInfoPrinter* pPrinter = new WinSalInfoPrinter;
    if (!rQueueInfo.moPortName)
        GetPrinterQueueState(&rQueueInfo);
    pPrinter->maDriverName = rQueueInfo.maDriver;
    pPrinter->maDeviceName = rQueueInfo.maPrinterName;
    pPrinter->maPortName = rQueueInfo.moPortName ? *rQueueInfo.moPortName : OUString();

    // check if the provided setup data match the actual printer
    ImplTestSalJobSetup(pPrinter, &rSetupData, true);

    HDC hDC = ImplCreateSalPrnIC(pPrinter, &rSetupData);
    if (!hDC)
    {
        delete pPrinter;
        return nullptr;
    }

    pPrinter->setHDC(hDC);
    if (!rSetupData.GetDriverData())
        ImplUpdateSalJobSetup(pPrinter, &rSetupData, false, nullptr);
    ImplDevModeToJobSetup(pPrinter, &rSetupData, JobSetFlags::ALL);
    rSetupData.SetSystem(JOBSETUP_SYSTEM_WINDOWS);

    return pPrinter;
}

std::unique_ptr<SalPrinter> WindowsInstance::CreatePrinter(SalInfoPrinter* pInfoPrinter)
{
    WinSalPrinter* pPrinter = new WinSalPrinter;
    pPrinter->mpInfoPrinter = static_cast<WinSalInfoPrinter*>(pInfoPrinter);
    return std::unique_ptr<SalPrinter>(pPrinter);
}

void WindowsInstance::GetPrinterQueueInfo(ImplPrnQueueList& rList)
{
    DWORD i;
    DWORD nBytes = 0;
    DWORD nInfoPrn4 = 0;
    // DocSkill: enumerate only local printers; PRINTER_ENUM_CONNECTIONS causes
    // Windows to attempt connecting to offline network printers at startup,
    // showing a blocking system "Waiting for printer" dialog.
    EnumPrintersW(PRINTER_ENUM_LOCAL, nullptr, 4, nullptr, 0, &nBytes,
                  &nInfoPrn4);
    if (!nBytes)
        return;

    PRINTER_INFO_4W* pWinInfo4 = static_cast<PRINTER_INFO_4W*>(std::malloc(nBytes));
    assert(pWinInfo4 && "Don't handle OOM conditions");
    if (EnumPrintersW(PRINTER_ENUM_LOCAL, nullptr, 4,
                      reinterpret_cast<LPBYTE>(pWinInfo4), nBytes, &nBytes, &nInfoPrn4))
    {
        for (i = 0; i < nInfoPrn4; i++)
        {
            std::unique_ptr<SalPrinterQueueInfo> pInfo(new SalPrinterQueueInfo);
            pInfo->maPrinterName = o3tl::toU(pWinInfo4[i].pPrinterName);
            pInfo->mnStatus = PrintQueueFlags::NONE;
            pInfo->mnJobs = 0;
            rList.Add(std::move(pInfo));
        }
    }
    std::free(pWinInfo4);
}

void WindowsInstance::GetPrinterQueueState(SalPrinterQueueInfo* pInfo)
{
    HANDLE hPrinter = nullptr;
    LPWSTR pPrnName = const_cast<LPWSTR>(o3tl::toW(pInfo->maPrinterName.getStr()));
    if (!OpenPrinterW(pPrnName, &hPrinter, nullptr))
        return;

    DWORD nBytes = 0;
    GetPrinterW(hPrinter, 2, nullptr, 0, &nBytes);
    if (nBytes)
    {
        PRINTER_INFO_2W* pWinInfo2 = static_cast<PRINTER_INFO_2W*>(std::malloc(nBytes));
        assert(pWinInfo2 && "Don't handle OOM conditions");
        if (GetPrinterW(hPrinter, 2, reinterpret_cast<LPBYTE>(pWinInfo2), nBytes, &nBytes))
        {
            if (pWinInfo2->pDriverName)
                pInfo->maDriver = o3tl::toU(pWinInfo2->pDriverName);
            OUString aPortName;
            if (pWinInfo2->pPortName)
                aPortName = o3tl::toU(pWinInfo2->pPortName);
            // pLocation can be 0 (the Windows docu doesn't describe this)
            if (pWinInfo2->pLocation && *pWinInfo2->pLocation)
                pInfo->maLocation = o3tl::toU(pWinInfo2->pLocation);
            else
                pInfo->maLocation = aPortName;
            // pComment can be 0 (the Windows docu doesn't describe this)
            if (pWinInfo2->pComment)
                pInfo->maComment = o3tl::toU(pWinInfo2->pComment);
            pInfo->mnStatus = ImplWinQueueStatusToSal(pWinInfo2->Status);
            pInfo->mnJobs = pWinInfo2->cJobs;
            if (!pInfo->moPortName)
                pInfo->moPortName = aPortName;
        }
        std::free(pWinInfo2);
    }
    ClosePrinter(hPrinter);
}

OUString WindowsInstance::GetDefaultPrinter()
{
    DWORD nChars = 0;
    GetDefaultPrinterW(nullptr, &nChars);
    if (nChars)
    {
        std::vector<WCHAR> pStr(nChars);
        if (GetDefaultPrinterW(pStr.data(), &nChars))
            return OUString(o3tl::toU(pStr.data()));
    }
    return OUString();
}

typedef LONG NTSTATUS;
typedef NTSTATUS(WINAPI* RtlGetVersion_t)(PRTL_OSVERSIONINFOW);
constexpr NTSTATUS STATUS_SUCCESS = 0x00000000;

OUString WindowsInstance::getWinArch()
{
    USHORT nNativeMachine = IMAGE_FILE_MACHINE_UNKNOWN;

    using LPFN_ISWOW64PROCESS2 = BOOL(WINAPI*)(HANDLE, USHORT*, USHORT*);
    auto fnIsWow64Process2 = reinterpret_cast<LPFN_ISWOW64PROCESS2>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "IsWow64Process2"));
    if (fnIsWow64Process2)
        fnIsWow64Process2(GetCurrentProcess(), &o3tl::temporary(USHORT()), &nNativeMachine);

    if (nNativeMachine == IMAGE_FILE_MACHINE_UNKNOWN)
    {
#if _WIN64
        nNativeMachine = IMAGE_FILE_MACHINE_AMD64;
#else
        BOOL isWow64 = FALSE;
        IsWow64Process(GetCurrentProcess(), &isWow64);

        if (isWow64)
            nNativeMachine = IMAGE_FILE_MACHINE_AMD64; // 32-bit process on 64-bit Windows
        else
            nNativeMachine = IMAGE_FILE_MACHINE_I386;

#endif
    }

    switch (nNativeMachine)
    {
        case IMAGE_FILE_MACHINE_I386:
            return u" X86_32"_ustr;
        case IMAGE_FILE_MACHINE_R3000:
            return u" R3000"_ustr;
        case IMAGE_FILE_MACHINE_R4000:
            return u" R4000"_ustr;
        case IMAGE_FILE_MACHINE_R10000:
            return u" R10000"_ustr;
        case IMAGE_FILE_MACHINE_WCEMIPSV2:
            return u" WCEMIPSV2"_ustr;
        case IMAGE_FILE_MACHINE_ALPHA:
            return u" ALPHA"_ustr;
        case IMAGE_FILE_MACHINE_SH3:
            return u" SH3"_ustr;
        case IMAGE_FILE_MACHINE_SH3DSP:
            return u" SH3DSP"_ustr;
        case IMAGE_FILE_MACHINE_SH3E:
            return u" SH3E"_ustr;
        case IMAGE_FILE_MACHINE_SH4:
            return u" SH4"_ustr;
        case IMAGE_FILE_MACHINE_SH5:
            return u" SH5"_ustr;
        case IMAGE_FILE_MACHINE_ARM:
            return u" ARM"_ustr;
        case IMAGE_FILE_MACHINE_THUMB:
            return u" THUMB"_ustr;
        case IMAGE_FILE_MACHINE_ARMNT:
            return u" ARMNT"_ustr;
        case IMAGE_FILE_MACHINE_AM33:
            return u" AM33"_ustr;
        case IMAGE_FILE_MACHINE_POWERPC:
            return u" POWERPC"_ustr;
        case IMAGE_FILE_MACHINE_POWERPCFP:
            return u" POWERPCFP"_ustr;
        case IMAGE_FILE_MACHINE_IA64:
            return u" IA64"_ustr;
        case IMAGE_FILE_MACHINE_MIPS16:
            return u" MIPS16"_ustr;
        case IMAGE_FILE_MACHINE_ALPHA64:
            return u" ALPHA64"_ustr;
        case IMAGE_FILE_MACHINE_MIPSFPU:
            return u" MIPSFPU"_ustr;
        case IMAGE_FILE_MACHINE_MIPSFPU16:
            return u" MIPSFPU16"_ustr;
        case IMAGE_FILE_MACHINE_TRICORE:
            return u" TRICORE"_ustr;
        case IMAGE_FILE_MACHINE_CEF:
            return u" CEF"_ustr;
        case IMAGE_FILE_MACHINE_EBC:
            return u" EBC"_ustr;
        case IMAGE_FILE_MACHINE_AMD64:
            return u" X86_64"_ustr;
        case IMAGE_FILE_MACHINE_M32R:
            return u" M32R"_ustr;
        case IMAGE_FILE_MACHINE_ARM64:
            return u" ARM64"_ustr;
        case IMAGE_FILE_MACHINE_CEE:
            return u" CEE"_ustr;
        default:
            assert(!"Yet unhandled case");
            return OUString();
    }
}

OUString WindowsInstance::getOSVersionString(DWORD nBuildNumber)
{
    OUStringBuffer result = u"Windows";
    if (nBuildNumber >= 22000)
        result.append(" 11");
    else if (nBuildNumber > 0)
        result.append(" 10");
    else // We don't know what Windows it is
        result.append(" unknown");

    result.append(getWinArch());

    if (nBuildNumber)
        result.append(" (build " + OUString::number(nBuildNumber) + ")");

    return result.makeStringAndClear();
}

DWORD WindowsInstance::getWindowsBuildNumber()
{
    static const DWORD nResult = [] {
        DWORD nBuildNumber = 0;
        // use RtlGetVersion to get build number
        if (HMODULE h_ntdll = GetModuleHandleW(L"ntdll.dll"))
        {
            if (auto RtlGetVersion
                = reinterpret_cast<RtlGetVersion_t>(GetProcAddress(h_ntdll, "RtlGetVersion")))
            {
                RTL_OSVERSIONINFOW vi2{}; // initialize with zeroes - a better alternative to memset
                vi2.dwOSVersionInfoSize = sizeof(vi2);
                if (STATUS_SUCCESS == RtlGetVersion(&vi2))
                {
                    nBuildNumber = vi2.dwBuildNumber;
                }
            }
        }
        return nBuildNumber;
    }();
    return nResult;
}

OUString WindowsInstance::getOSVersion() { return getOSVersionString(getWindowsBuildNumber()); }

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
