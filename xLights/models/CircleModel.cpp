
#include <wx/xml/xml.h>

#include "CircleModel.h"

CircleModel::CircleModel(wxXmlNode *node, const NetInfoClass &netInfo, bool zeroBased)
{
    SetFromXml(node, netInfo, zeroBased);
}

CircleModel::~CircleModel()
{
}


int CircleModel::GetStrandLength(int strand) const {
    return SingleNode ? 1 : circleSizes[strand];
}
int CircleModel::MapToNodeIndex(int strand, int node) const {
    int idx = 0;
    for (int x = 0; x < strand; x++) {
        idx += circleSizes[x];
    }
    idx += node;
    return idx;
}
int CircleModel::GetNumStrands() const {
    return circleSizes.size();
}


// parm3 is number of points
// top left=top ccw, top right=top cw, bottom left=bottom cw, bottom right=bottom ccw

void CircleModel::InitModel() {
    if( ModelXml->HasAttribute("circleSizes") )
    {
        wxString tempstr=ModelXml->GetAttribute("circleSizes");
        circleSizes.resize(0);
        while (tempstr.size() > 0) {
            wxString t2 = tempstr;
            if (tempstr.Contains(",")) {
                t2 = tempstr.SubString(0, tempstr.Find(","));
                tempstr = tempstr.SubString(tempstr.Find(",") + 1, tempstr.length());
            } else {
                tempstr = "";
            }
            long i2 = 0;
            t2.ToLong(&i2);
            if ( i2 > 0) {
                circleSizes.resize(circleSizes.size() + 1);
                circleSizes[circleSizes.size() - 1] = i2;
            }
        }
    }
    InitCircle();
    SetCircleCoord();
}

void CircleModel::InitCircle() {
    int maxLights = 0;
    int numLights = parm1 * parm2;
    int cnt = 0;
    
    if (circleSizes.size() == 0) {
        circleSizes.resize(1);
        circleSizes[0] = numLights;
    }
    for (int x = 0; x < circleSizes.size(); x++) {
        if ((cnt + circleSizes[x]) > numLights) {
            circleSizes[x] = numLights - cnt;
        }
        cnt += circleSizes[x];
        if (circleSizes[x] > maxLights) {
            maxLights = circleSizes[x];
        }
    }
    
    SetNodeCount(parm1,parm2,rgbOrder);
    SetBufferSize(circleSizes.size(),maxLights);
    int LastStringNum=-1;
    int chan = 0,idx;
    int ChanIncr=SingleChannel ?  1 : 3;
    size_t NodeCount=GetNodeCount();
    
    int node = 0;
    int nodesToMap = NodeCount;
    for (int circle = 0; circle < circleSizes.size(); circle++) {
        idx = 0;
        int loop_count = std::min(nodesToMap, circleSizes[circle]);
        for(size_t n=0; n<loop_count; n++) {
            if (Nodes[node]->StringNum != LastStringNum) {
                LastStringNum=Nodes[node]->StringNum;
                chan=stringStartChan[LastStringNum];
            }
            Nodes[node]->ActChan=chan;
            chan+=ChanIncr;
            double pct = (loop_count == 1) ? (double)n : (double)n / (double)(loop_count-1);
            size_t CoordCount=GetCoordCount(node);
            for(size_t c=0; c < CoordCount; c++) {
                int x_pos = (circle == 0) ? idx : (int)(pct*(double)(maxLights-1));
                Nodes[node]->Coords[c].bufX=x_pos;
                Nodes[node]->Coords[c].bufY=circle;
                idx++;
            }
            node++;
        }
        nodesToMap -= loop_count;
    }
}

// Set screen coordinates for circles
void CircleModel::SetCircleCoord() {
    double x,y;
    size_t NodeCount=GetNodeCount();
    SetRenderSize(circleSizes[0]*2,circleSizes[0]*2);
    int nodesToMap = NodeCount;
    int node = 0;
    double maxRadius = RenderWi / 2.0;
    double minRadius = (double)parm3/100.0 * maxRadius;
    for (int circle = 0; circle < circleSizes.size(); circle++) {
        int loop_count = std::min(nodesToMap, circleSizes[circle]);
        double midpt=loop_count;
        midpt -= 1.0;
        midpt /= 2.0;
        double radius = (circleSizes.size() == 1) ? maxRadius : (double)minRadius + (maxRadius-minRadius)*(1.0-(double)circle/(double)(circleSizes.size()-1));
        for(size_t n=0; n<loop_count; n++) {
            size_t CoordCount=GetCoordCount(node);
            for(size_t c=0; c < CoordCount; c++) {
                double angle=-M_PI + M_PI * ((loop_count==1) ? 1 : (double)n / (double)loop_count) * 2.0;
                x=sin(angle)*radius;
                y=cos(angle)*radius;
                Nodes[node]->Coords[c].screenX=x;
                Nodes[node]->Coords[c].screenY=y;
            }
            node++;
        }
        nodesToMap -= loop_count;
    }
}