#pragma once
#include "resource.h"
#include <chrono>

#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/objdetect/objdetect_c.h"
#include "opencv2/imgproc/types_c.h"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/video/background_segm.hpp"

using namespace std;
using namespace cv;
using namespace std::chrono;

class CMainDialog : public CDialogEx
{

public:
	CMainDialog(CWnd* pParent = nullptr);	// standard constructor
    virtual ~CMainDialog();
	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG };
#endif

	afx_msg void OnExit();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	void RenderThread();

	DECLARE_MESSAGE_MAP()
    
    bool Running() {
        return m_Running;
    }

    void  Running(bool value) {
        m_Running = value;
    }
    
private:

    bool show_faces = false;
    bool show_faces_history = false;
    bool show_all_objects = false;
    bool freez_on_alert = false;
	thread render_thread;
	bool m_Running = true;
	bool m_Capturing = true;
    Mat frame, OriginalFrame;
    cv::CascadeClassifier face_cascade;
    vector<cv::Rect> faces;
    list<vector<cv::Rect>>faces_history;
    vector<cv::Rect> _bbox_history;
    vector<cv::Rect> _bbox_old_history;
    vector<vector<cv::Point>> _regions_;

public:
    afx_msg void OnBnClickedStartStopCapture();
};