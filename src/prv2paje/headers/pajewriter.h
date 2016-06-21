#ifndef PAJEWRITER_H
#define PAJEWRITER_H

#include <cstdlib>
#include <iostream>
#include <ctime>

#include <fstream>
#include <map>
#include <poti.h>

#include "config.h"
#include "prvmetadata.h"
#include "pcfparser.h"
#include "pajepending.h"
#include "message.h"

using namespace std;

namespace prv2paje{

    class PajeWriter
    {
    public:
        PajeWriter(string pajePath);
        ~PajeWriter();
        void pushEvents(int cpu, int app, int task, int thread, double timestamp, map<int, string>* events, long lineNumber);
        void pushState(int cpu, int app, int task, int thread, double startTimestamp, double endTimestamp, string value, long lineNumber);
        void generatePajeHeader();
        void defineAndCreatePajeContainers();
        void definePajeEvents();
        void finalize();
        PrvMetaData *getPrvMetaData() const;
        void setPrvMetaData(PrvMetaData *value);

        PcfParser *getPcfParser() const;
        void setPcfParser(PcfParser *value);

    private:
        void checkContainerChain(long int timestamp, int cpu, int app, int task, int thread);

        string pajePath;
        PrvMetaData *prvMetaData;
        PcfParser *pcfParser;
        PajePending pajePending;
        vector<map<int, map<int, map<int, bool > > > > containerChain;
    };

}

#endif // PAJEWRITER_H
