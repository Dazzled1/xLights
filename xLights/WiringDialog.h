#ifndef WIRINGDIALOG_H
#define WIRINGDIALOG_H

//(*Headers(WiringDialog)
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/dialog.h>
//*)

#include <wx/grid.h>
#include <wx/bitmap.h>
#include <map>
#include <list>

class WiringDialog: public wxDialog
{
    wxString _modelname;
    wxBitmap bmp;
    wxBitmap sizedbmp;
    void RenderMultiLight(std::map<int, std::list<wxPoint>>& points, int width, int height);
    void RenderNodes(std::map<int, std::list<wxPoint>>& points, int width, int height);
    std::map<int, std::list<wxPoint>> ExtractPoints(wxGrid* grid, bool reverse);
    void ResizeBitmap(void);
    void RightClick(wxContextMenuEvent& event);
    void OnPopup(wxCommandEvent& event);
    static const long ID_MNU_EXPORT;

    public:

		WiringDialog(wxWindow* parent, wxGrid* grid, bool reverse, wxString modelname,wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~WiringDialog();

		//(*Declarations(WiringDialog)
		wxStaticBitmap* StaticBitmap_Wiring;
		//*)

	protected:

		//(*Identifiers(WiringDialog)
		static const long ID_STATICBITMAP1;
		//*)

	private:

		//(*Handlers(WiringDialog)
		void OnResize(wxSizeEvent& event);
		//*)

		DECLARE_EVENT_TABLE()
};

#endif
