#include "FPP.h"
#include <wx/msgdlg.h>
#include <wx/sstream.h>
#include <wx/regex.h>
#include <wx/file.h>
#include <wx/filename.h>
#include "xLightsXmlFile.h"
#include "models/Model.h"
#include "outputs/OutputManager.h"
#include "outputs/Output.h"
#include <log4cpp/Category.hh>

FPP::FPP(OutputManager* outputManager, const std::string& ip, const std::string& user, const std::string& password)
{
    _outputManager = outputManager;
 	_user = user;
	_password = password;
	_ip = ip;

	_connected = _http.Connect(_ip);

    if (_connected)
    {
        std::string version = GetURL("//");

        //Version: <a href = 'about.php' class = 'nonULLink'>v1.6 - 25 - gd87f066
        static wxRegEx versionregex("(Version: .*?nonULLink..v)([0-9]+\\.[0-9]+)", wxRE_ADVANCED | wxRE_NEWLINE);
        if (versionregex.Matches(wxString(version)))
        {
            _version = versionregex.GetMatch(wxString(version), 2).ToStdString();
        }

        _ftp.Connect(ip, user, password);
    }
}

FPP::FPP(OutputManager* outputManager)
{
    _outputManager = outputManager;
    _connected = false;
}

FPP::~FPP()
{
    _http.Close();
}

std::string FPP::GetURL(const std::string& url, bool logresult)
{
    static log4cpp::Category &logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    wxString res;
    _http.SetMethod("GET");
    wxInputStream *httpStream = _http.GetInputStream(wxString(url));
    logger_base.debug("Making request to fpp '%s'.", (const char *)url.c_str());

    if (_http.GetError() == wxPROTO_NOERR)
    {
        wxStringOutputStream out_stream(&res);
        httpStream->Read(out_stream);

        if (logresult)
        {
            logger_base.debug("Response from fpp '%s'.", (const char *)res.c_str());
        }
    }
    else
    {
        logger_base.error("Unable to connect to fpp '%s'.", (const char *)url.c_str());
        wxMessageBox(_T("Unable to connect!"));
    }

    wxDELETE(httpStream);
    return res.ToStdString();
}

void FPP::RestartFFPD()
{
    GetURL("//fppxml.php?command=restartFPPD");
}

void FPP::E131Output(bool enable)
{
    if (enable)
    {
        GetURL("//fppjson.php?command=setSetting&plugin=&key=E131Enabled&value=1");
    }
    else
    {
        GetURL("//fppjson.php?command=setSetting&plugin=&key=E131Enabled&value=0");
    }
}

bool FPP::SetInputUniversesBridge(std::list<int>& selected, wxWindow* parent)
{
    if (_ftp.IsConnected())
    {
        // now create a universes file
        std::string file = SaveFPPUniverses(_ip, selected, false);

        bool cancelled = _ftp.UploadFile(file, "/home/fpp/media", "universes", true, false, parent);

        // deactive outputs to these inputs
        E131Output(false);

        // restart ffpd
        RestartFFPD();

        ::wxRemoveFile(wxString(file));

        return cancelled;
    }

    return true;
}

bool FPP::SetOutputUniversesPlayer(wxWindow* parent)
{
    if (_ftp.IsConnected())
    {
        // now create a universes file
        std::string file = SaveFPPUniverses("", std::list<int>(), false);

        bool cancelled = _ftp.UploadFile(file, "/home/fpp/media", "universes", true, false, parent);

        // active outputs
        E131Output(true);

        // restart ffpd
        RestartFFPD();

        ::wxRemoveFile(wxString(file));

        return cancelled;
    }

    return true;
}

bool FPP::SetChannelMemoryMaps(ModelManager* allmodels, wxWindow* parent)
{
    if (_ftp.IsConnected())
    {
        // now create a universes file
        std::string file = SaveFPPChannelMemoryMaps(allmodels);

        bool cancelled = _ftp.UploadFile(file, "/home/fpp/media", "channelmemorymaps", true, false, parent);

        // restart ffpd
        RestartFFPD();

        ::wxRemoveFile(wxString(file));

        return cancelled;
    }

    return true;
}

std::string FPP::SaveFPPChannelMemoryMaps(ModelManager* allmodels) const
{
    static log4cpp::Category &logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    wxFileName fn;
    fn.AssignTempFileName("channelmemorymaps");
    std::string file = fn.GetFullName().ToStdString();

    logger_base.debug("FPP models memory map file written to %s.", (const char *)file.c_str());

    wxFile channelmemorymaps;
    channelmemorymaps.Open(file, wxFile::write);

    if (channelmemorymaps.IsOpened())
    {
        for (auto m = allmodels->begin(); m != allmodels->end(); ++m)
        {
            Model* model = m->second;
            wxString stch = model->GetModelXml()->GetAttribute("StartChannel", wxString::Format("%d?", model->NodeStartChannel(0) + 1)); //NOTE: value coming from model is probably not what is wanted, so show the base ch# instead
            int ch = model->GetNumberFromChannelString(model->ModelStartChannel);
            wxString name(model->name);
            name.Replace(" ", "_");
            if (model->GetNumStrands() > 0) {
                channelmemorymaps.Write(wxString::Format("%s,%d,%d,horizontal,TL,%d,%d\n",
                    name,
                    ch,
                    model->GetActChanCount(),
                    model->GetNumStrands(),
                    1));
            }
        }
        channelmemorymaps.Close();
    }

    return file;
}

std::string FPP::SaveFPPUniverses(const std::string& onlyip, const std::list<int>& selected, bool onebased) const
{
    static log4cpp::Category &logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    wxFileName fn;
    fn.AssignTempFileName("universes");
    std::string file = fn.GetFullName().ToStdString();

    logger_base.debug("FPP universes file written to %s.", (const char *)file.c_str());

    wxFile universes;
    universes.Open(file, wxFile::write);

    if (universes.IsOpened())
    {
        // Get universes based on IP
        std::list<Output*> outputs = _outputManager->GetAllOutputs(onlyip);

        // get outputs based on selected
        std::list<Output*> o2 = _outputManager->GetAllOutputs(selected);

        // now merge them together and eleminate the duplicates
        outputs.merge(o2);
        outputs.sort();
        outputs.unique();

        long onebasedcount = 1;

        for (auto it = outputs.begin(); it != outputs.end(); ++it)
        {
            if ((*it)->GetType() == OUTPUT_E131)
            {
                int c = (*it)->GetStartChannel();

                if (onebased)
                {
                    c = onebasedcount;
                }

                if ((*it)->GetIP() == "MULTICAST")
                {
                    universes.Write("1," + (*it)->GetUniverseString() + "," + wxString::Format(wxT("%i"), c).ToStdString() + "," + wxString::Format(wxT("%i"), (*it)->GetChannels()).ToStdString() + ",0,,\r\n");
                }
                else
                {
                    universes.Write("1," + (*it)->GetUniverseString() + "," + wxString::Format(wxT("%i"), c).ToStdString() + "," + wxString::Format(wxT("%i"), (*it)->GetChannels()).ToStdString() + ",1," + (*it)->GetIP() + ",\r\n");
                }
                onebasedcount += (*it)->GetChannels();
            }
        }

        universes.Close();
    }

    return file;
}

bool FPP::UploadSequence(std::string file, wxWindow* parent)
{
    bool cancelled = false;
    wxString media = "";

    wxXmlDocument doc(file);
    if (doc.IsOk())
    {
        wxXmlNode* root = doc.GetRoot();
        if (root->GetName() == "xsequence")
        {
            for (auto n = root->GetChildren(); n != nullptr; n = n->GetNext())
            {
                if (n->GetName() == "head")
                {
                    for (auto n1 = n->GetChildren(); n1 != nullptr; n1 = n1->GetNext())
                    {
                        if (n1->GetName() == "mediaFile")
                        {
                            media = n1->GetNodeContent();
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    wxFileName fn(file);
    wxString fseq = fn.GetPath() + "/" + fn.GetName() + ".fseq";
    if (wxFile::Exists(fseq))
    {
        cancelled = _ftp.UploadFile(fseq.ToStdString(), "/home/fpp/media/sequences", fn.GetName().ToStdString() + ".fseq", false, true, parent);
    }

    if (!cancelled && media != "")
    {
        media = xLightsXmlFile::FixFile("", media);
        wxFileName fnmedia(media);
        cancelled = _ftp.UploadFile(media.ToStdString(), "/home/fpp/media/music", fnmedia.GetName().ToStdString() + "." + fnmedia.GetExt().ToStdString(), false, true, parent);
    }

    return cancelled;
}
