#include "stdafx.h"
#include "Detector.h"
#include "MainForm.h"

#pragma comment(lib,"xaudio2.lib")
#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "comctl32.lib" )

#define FRAME_WIDTH  640
#define FRAME_HEIGHT 480
#define FPS          220
#define history      5

void DoEvents()
{
	MSG msg;
	BOOL result;

	while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		result = ::GetMessage(&msg, NULL, 0, 0);
		if (result == 0) // WM_QUIT
		{
			::PostQuitMessage(static_cast<int>(msg.wParam));
			break;
		}
		else if (result == -1)
		{
		}
		else
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
}

//Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK)
    {
        DWORD dwRead;
        if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        switch (dwChunkType)
        {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        default:
            if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                return HRESULT_FROM_WIN32(GetLastError());
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc)
        {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;

        if (bytesRead >= dwRIFFDataSize) return S_FALSE;

    }

    return S_OK;

}

HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD dwRead;
    if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
        hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}


CMainDialog::CMainDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG, pParent)
{
    //Creating a instance od XAufio2 Engine
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (hr == S_OK && FAILED(hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        OutputDebugString( "XAudio2Create Failed!\n");
    }

    if (hr == S_OK && FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasteringVoice)))
        OutputDebugString(  "XAudio2Create Failed!\n");


    //Open the audio file with CreateFile

    HANDLE hFile = CreateFile(
        "beep.wav",
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    DWORD dwChunkPosition = 0;
    DWORD dwChunkSize;


    if (INVALID_HANDLE_VALUE == hFile)
        OutputDebugString(  "INVALID HANDLE VALUE!\n");
    //return HRESULT_FROM_WIN32(GetLastError());
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
    {
        OutputDebugString(  "INVALID SET FILE POINTER!\n");
        //return HRESULT_FROM_WIN32(GetLastError());
    }

    OutputDebugString(  "Locating a 'RIFF' and whole data size\n");
    FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);

    OutputDebugString(  "Checking a filetype\n");
    DWORD filetype;
    ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);

    if (filetype != fourccWAVE)
        OutputDebugString(  "This isn't WAVE file!\n");
    else OutputDebugString(  "WAVE format!\n");

    OutputDebugString(  "Locating a fmt and filling out WAVEFORMATEXTENSIBLE structure\n");
    FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
    ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition);

    OutputDebugString(  "Locating a DATA\n");
    FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);

    OutputDebugString(  "Reading a sound\n");
    BYTE * pDataBuffer = new BYTE[dwChunkSize];
    ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);
    buffer.AudioBytes = dwChunkSize;
    buffer.pAudioData = pDataBuffer;
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    //Plaing preloaded sound;
    if (hr == S_OK)
        hr = pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&wfx);



    if (!face_cascade.load("haarcascade_frontalface_alt.xml")) {
        MessageBoxA("File \"haarcascade_frontalface_alt.xml\" cannot be found!", "Error", MB_OK | MB_ICONERROR);
        exit(-1);
    }
}

CMainDialog::~CMainDialog() 
{
    Running(false);
    DoEvents();
    if (render_thread.joinable()) {
        render_thread.join();
    }
}

void CMainDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMainDialog, CDialogEx)
	ON_BN_CLICKED(IDExit, OnExit)
    ON_BN_CLICKED(IDScreenCapture, &CMainDialog::OnBnClickedStartStopCapture)
END_MESSAGE_MAP()

    
// CMFCApplicationTestDlg message handlers

BOOL CMainDialog::OnInitDialog() 
{
	CDialogEx::OnInitDialog();
    render_thread = std::thread(&CMainDialog::RenderThread, this);
	return TRUE;
}



void CMainDialog::OnExit() 
{
    CDialogEx::OnOK();
}


cv::Point Center(cv::Rect value)
{
    return((value.br() + value.tl()) * 0.5);
}



void CMainDialog::RenderThread() 
{
    std::string des_ = "";

    if (face_cascade.empty()) return;

    VideoCapture cap;
    cap.set(CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cap.set(CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
    cap.set(CAP_PROP_FPS, FPS);

    if (!cap.open(0)) return;
    RECT monitorRect;
    ::GetWindowRect(::GetDesktopWindow(), &monitorRect);

    float varThreshold = 16.f; 
    bool bShadowDetection = true;
    Ptr<BackgroundSubtractor> pMOG2= createBackgroundSubtractorMOG2(history, varThreshold, bShadowDetection);
    Ptr<MSER> mser = MSER::create();
    Mat1b fmask;
    vector<cv::Rect> Prev_faces;
    bool reset_frame = false;

	while (Running()) {
        if (!m_Capturing) {
            DoEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(256));
            continue;
        }
 
        des_ = "";
        auto begin = chrono::high_resolution_clock::now();

        if (cap.read(frame)) {
            OriginalFrame = frame.clone();

            //keep previous frame faces info for comparing if face movement happend
            if (!faces.empty()) 
                Prev_faces = faces;
            
//            cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
//            cv::GaussianBlur(frame, frame, cv::Size(5, 5), 15, 15, 4);

            face_cascade.detectMultiScale(frame, faces, 1.1, 1, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(10, 10));

            if (faces.empty() && !Prev_faces.empty())
                faces = Prev_faces;

            if (!faces.empty()) {


                faces_history.push_front(faces);
                while (faces_history.size() > history)
                    faces_history.pop_back();

                if (reset_frame) {
                    rectangle(fmask, cv::Rect(0, 0, fmask.size().width, fmask.size().height), Scalar(0, 0, 0), FILLED, 8, 0);
                    pMOG2->apply(frame, fmask, 100);
                    reset_frame = false;
                }

                pMOG2->apply(frame, fmask, -1);


                vector<cv::Rect> _bbox_;
                mser->detectRegions(fmask, _regions_, _bbox_);

                //removing objects in the faces
                vector<cv::Rect> reduced_box_;
                for (int faceIndex = 0; faceIndex < faces.size(); faceIndex++) {
                    for (int boxIndex = 0; boxIndex < _bbox_.size(); boxIndex++) {
                        if ((_bbox_[boxIndex] & faces[faceIndex]).area() == 0)
                            reduced_box_.push_back(_bbox_[boxIndex]);
                    }
                }

                _bbox_ = reduced_box_;
                reduced_box_.clear();

                for (int boxIndex = 0; boxIndex < _bbox_.size(); boxIndex++) 
                    for (int iboxIndex = boxIndex; iboxIndex < _bbox_.size(); iboxIndex++)
                        if (boxIndex != iboxIndex &&
                           (((_bbox_[boxIndex] & _bbox_[iboxIndex]).area() != 0)) ||
                            (cv::norm(Center(_bbox_[boxIndex])-Center(_bbox_[iboxIndex]))<100) 
                           )
                            reduced_box_.push_back(_bbox_[boxIndex]| _bbox_[iboxIndex]);

//                for (int i = 0; i < _bbox_.size(); i++)
//                    rectangle(OriginalFrame, _bbox_[i], CV_RGB(255, 255, 255), 1, 8, 0);

                _bbox_.clear();

                if (_bbox_history.empty()) 
                    // new boxes apepared and there were no box in history
                    _bbox_history = reduced_box_;
                else
                {
                    for (int faceIndex = 0; faceIndex < faces.size(); faceIndex++) {

                        int face_move_dist = 0;
                        if (Prev_faces.size() > faceIndex)
                            face_move_dist = cv::norm(Center(faces[faceIndex]) - Center(Prev_faces[faceIndex]));
                        cv::Point pf = Center(faces[faceIndex]);

                        for (int boxIndex = 0; boxIndex < reduced_box_.size(); boxIndex++) {

                            for (int boxHIndex = 0; boxHIndex < _bbox_history.size(); boxHIndex++) {
                                //filter the trackable boxes 

                                    cv::Point p1 = Center(reduced_box_[boxIndex]);
                                    cv::Point p2 = Center(_bbox_history[boxHIndex]);

                                    if (cv::norm(pf - p1) >= cv::norm(pf - p2) || //getting far from face
                                        cv::norm(pf - p1) <= cv::norm(p2 - p1)) //or wrong objects
                                        continue;

                                    double a = p1.y - p2.y;
                                    double b = p2.x - p1.x;
                                    double c = p1.x * p2.y - p2.x * p1.y;
                                    double  dist = abs(a * pf.x + b * pf.y + c) / sqrt(a * a + b * b);

                                    double point_distance = cv::norm(p1 - p2);

                                    if (point_distance >= face_move_dist  &&
                                        dist < 30) {
 //                                       line(OriginalFrame, p1, p2, Scalar(255, 255, 255), 3, LINE_8);
 //                                       rectangle(OriginalFrame, reduced_box_[boxIndex], CV_RGB(255, 255, 255), 3, 8, 0);
 //                                       rectangle(OriginalFrame, _bbox_history[boxHIndex], CV_RGB(255, 0, 255), 3, 8, 0);
                                        _bbox_.push_back(reduced_box_[boxIndex]);
                                    }

                            }

                            if(!_bbox_old_history.empty() )
                            for (int boxIndex = 0; boxIndex < _bbox_.size(); boxIndex++) {
                                    bool alert = false;
                                    for (int boxHIndex = 0; boxHIndex < _bbox_old_history.size(); boxHIndex++) {
                                        cv::Point p1 = Center(_bbox_[boxIndex]);
                                        cv::Point p2 = Center(_bbox_old_history[boxHIndex]);

                                        if (cv::norm(pf - p1) >= cv::norm(pf - p2) || //getting far from face
                                            cv::norm(pf - p1) <= cv::norm(p2 - p1)) //or wrong objects
                                            continue;

                                        double a = p1.y - p2.y;
                                        double b = p2.x - p1.x;
                                        double c = p1.x * p2.y - p2.x * p1.y;
                                        double  dist = abs(a * pf.x + b * pf.y + c) / sqrt(a * a + b * b);

                                        double point_distance = cv::norm(p1 - p2);

                                        if (point_distance >= face_move_dist*2 &&
                                            point_distance < faces[faceIndex].height &&
                                            dist < 40) {


                                            des_ += "fmd: " + to_string(face_move_dist) + "\n";
                                            des_ += "pfl: " + to_string(dist) + "\n";
                                            des_ += "pfp: " + to_string(point_distance) + "\n";



                                            line(OriginalFrame, p1, p2, Scalar(255, 255, 255), 3, LINE_8);
                                            rectangle(OriginalFrame, _bbox_old_history[boxHIndex], CV_RGB(0, 0, 255), 2, 8, 0);
                                            rectangle(OriginalFrame, _bbox_[boxIndex], CV_RGB(0, 255, 0), 1, 8, 0);
                                            if (!freez_on_alert) {
                                                if (hr == S_OK)
                                                    hr = pSourceVoice->SubmitSourceBuffer(&buffer);
                                                if (hr == S_OK)
                                                    hr = pSourceVoice->Start(0);
                                                rectangle(OriginalFrame, cv::Rect(0, 0, OriginalFrame.size().width, OriginalFrame.size().height), Scalar(0, 0, 255), FILLED, 8, 0);
                                            }

                                            if (freez_on_alert) {
                                                m_Capturing = false;
                                            }
                                            reset_frame = true;
                                            alert = true;
                                            break;
                                        }
                                    }
                                    if (alert) {
                                        _bbox_history.clear();
                                        _bbox_old_history.clear();
                                    }
                            }

                            _bbox_old_history = _bbox_history;
                            _bbox_history = _bbox_;

                        }
                    }
                }
                cv::namedWindow("mask", cv::WINDOW_NORMAL);
                imshow("mask", fmask);
                cv::resizeWindow("mask", FRAME_WIDTH, FRAME_HEIGHT);
                cv::moveWindow("mask", monitorRect.right - FRAME_WIDTH - 2, FRAME_HEIGHT + 1);
                cv::setWindowProperty("mask", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);

                if(show_faces)
                for (int i = 0; i < faces.size(); i++) 
                    rectangle(OriginalFrame, faces[i], Scalar(0, 255, 255), 1, 8, 0);
                
                if(show_faces_history)
                for (vector<cv::Rect> fs : faces_history)
                    for (int i = 0; i < fs.size(); i++) 
                        rectangle(OriginalFrame, cv::Rect(Center(fs[i]).x - 1, Center(fs[i]).y - 1, 2, 2), Scalar(255, 255, 255), FILLED, 8, 0);
                else
                    for (int i = 0; i < faces.size(); i++)
                        rectangle(OriginalFrame, cv::Rect(Center(faces[i]).x - 1, Center(faces[i]).y - 1, 2, 2), Scalar(255, 255, 255), FILLED, 8, 0);

                if(show_all_objects)
                for (int j = 0; j < reduced_box_.size(); j++)
                        rectangle(OriginalFrame, reduced_box_[j], CV_RGB(0, 255, 255), 1, 8, 0);

            }

            cv::namedWindow("frame", cv::WINDOW_NORMAL);
            imshow("frame", OriginalFrame);
            cv::resizeWindow("frame", FRAME_WIDTH, FRAME_HEIGHT);
            cv::moveWindow("frame", monitorRect.right - FRAME_WIDTH - 2, 0);
            cv::setWindowProperty("frame", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
        }
                

		auto end = chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

		des_ += "ms: " + to_string(ms) + "\n";

        if (Running()) {
            SetDlgItemText(IDC_Description, des_.c_str());
        }

        DoEvents();
    }
}

void CMainDialog::OnBnClickedStartStopCapture()
{
    m_Capturing = !m_Capturing;
}
