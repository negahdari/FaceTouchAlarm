#ifndef DETECTOR_H
#define DETECTOR_H

// CIdentifyTest.h : main header file for the MFCApplicationTest application
//
#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include <gdiplus.h>

using namespace Gdiplus;
#pragma comment (lib, "gdiplus.lib")


// CIdentifyTest:
// See IdentifyTest.cpp for the implementation of this class
//

class CDetector : public CWinApp
{
public:
    CDetector();


    // Overrides
public:
    virtual BOOL InitInstance();

    // Implementation

public:
    DECLARE_MESSAGE_MAP()

private:
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;

};

extern CDetector theApp;

#endif // DETECTOR_H 
