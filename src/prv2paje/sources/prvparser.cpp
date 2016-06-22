#include "prvparser.h"


prv2paje::PrvParser::PrvParser(ifstream *prvStream, prv2paje::PcfParser *pcfParser, prv2paje::PajeWriter *pajeWriter):
    prvStream(prvStream), pcfParser(pcfParser), pajeWriter(pajeWriter), prvMetaData(new PrvMetaData())
{

}

prv2paje::PrvParser::~PrvParser()
{
    delete prvMetaData;
}

void prv2paje::PrvParser::parse()
{
    string line;
    long lineNumber=0;
    double currentTimestamp=0;
    Mode mode=Header;
    if (prvStream){
        while(getline(*prvStream,line)){
            lineNumber++;
            if (lineNumber%100000==0){
                Message::Debug(to_string(lineNumber)+ " lines processed");
            }
            replace(line.begin(), line.end(), '\t', ' ');
            std::size_t found = line.find_first_of("(");
            if ((found!=std::string::npos)&&(found+1<line.length())){
                found = line.find_first_of("a", found+1);
            }
            if ((found!=std::string::npos)&&(line[found+1]=='t')&&(found+5<line.length())&&(line[found+5]==':')){
                line[found+5]='*';
            }
            replace(line.begin(), line.end(), PRV_HEADER_QUOTE_IN_CHAR, GENERIC_SEP_CHAR);
            replace(line.begin(), line.end(), PRV_HEADER_QUOTE_OUT_CHAR, GENERIC_SEP_CHAR);
            trim_all(line);
            if (!line.empty()){
                escaped_list_separator<char> sep(GENERIC_ESCAPE_CHAR, PRV_HEADER_SEP_MAIN_CHAR, GENERIC_QUOTE_CHAR);
                tokenizer<escaped_list_separator<char> > tokens(line, sep);
                tokenizer<escaped_list_separator<char> >::iterator tokensIterator=tokens.begin();
                if (mode==Header){
                    //comment
                    Message::Info("Parsing Header", 2);
                    string temp=*tokensIterator;
                    erase_all(temp, GENERIC_SEP);
                    replace(temp.begin(), temp.end(), '*', ':');
                    prvMetaData->setComment(temp);
                    tokensIterator++;
                    Message::Info("Comment: " +temp, 3);
                    //duration_unit                
                    temp=*tokensIterator;
                    tokensIterator++;
                    vector<string> tokensTemp;
                    split(tokensTemp, temp, is_any_of(PRV_HEADER_SEP_DURATION));
                    if (tokensTemp.size()>0){
                        prvMetaData->setDuration(stol(tokensTemp.operator [](PRV_HEADER_SUBFIELD_DURATION_VALUE)));
                    }
                    if (tokensTemp.size()>1){
                        prvMetaData->setTimeUnit(tokensTemp[PRV_HEADER_SUBFIELD_DURATION_UNIT]);
                    }else{
                        prvMetaData->setTimeUnit("");
                    }
                    Message::Info("Duration: " +to_string(prvMetaData->getDuration())+", Unit: "+prvMetaData->getTimeUnit(), 3);
                    Message::Info("Time Divider: " +to_string(prvMetaData->getTimeDivider()), 3);
                    //nodes"<cpu>"
                    temp=*tokensIterator;
                    tokensIterator++;
                    //nodes;<cpu>;
                    tokensTemp.clear();
                    split(tokensTemp, temp, is_any_of(GENERIC_SEP));
                    //nodes
                    int nodes=atoi(tokensTemp.operator [](PRV_HEADER_SUBFIELD_HW_NODES).c_str());
                    prvMetaData->setNodes(nodes);
                    Message::Info("Number of nodes: " +to_string(prvMetaData->getNodes()), 3);
                    vector<int> * cpus = new vector<int>();
                    Message::Info("CPUS:", 3);
                    temp=tokensTemp.operator [](PRV_HEADER_SUBFIELD_HW_CPUS);
                    split(tokensTemp, temp, is_any_of(PRV_HEADER_SEP_HW_CPUS));
                    for (int i=0; i<tokensTemp.size(); i++){
                        cpus->push_back(atoi(tokensTemp.operator [](i).c_str()));
                        Message::Info("Node: "+to_string(i)+", CPU number: "+to_string(cpus->at(i)), 4);
                    }
                    prvMetaData->setCpus(cpus);
                    //drop what follows, not necessary to rebuild the hierarchy
                    //...
                    //saving metadata
                    Message::Info("Initializing writer", 2);
                    Message::Info("Storing metadata", 3);
                    pajeWriter->setPrvMetaData(prvMetaData);
                    Message::Info("Storing event types", 3);
                    pajeWriter->setPcfParser(pcfParser);
                    Message::Info("Generating header", 3);
                    pajeWriter->generatePajeHeader();
                    Message::Info("Define and create Paje containers", 3);
                    pajeWriter->defineAndCreatePajeContainers();
                    Message::Info("Define and create Paje event types", 3);
                    pajeWriter->definePajeEvents();
                    mode=Body;
                    Message::Info("Parsing Body", 2);
                }else{
                    string eventType=*tokensIterator;
                    tokensIterator++;
                    //communicator
                    if (eventType.compare(PRV_BODY_COMMUNICATOR)==0){
                        //do nothing TODO, low priority...
                    //communications
                    }else if (eventType.compare(PRV_BODY_COMMUNICATION)==0){
                        //TODO
                    //events
                    }else if (eventType.compare(PRV_BODY_EVENTS)==0){
                        string temp=*tokensIterator;
                        tokensIterator++;
                        int cpu=atoi(temp.c_str());
                        temp=*tokensIterator;
                        tokensIterator++;
                        int app=atoi(temp.c_str());
                        temp=*tokensIterator;
                        tokensIterator++;
                        int task=atoi(temp.c_str());
                        temp=*tokensIterator;
                        tokensIterator++;
                        int thread=atoi(temp.c_str());
                        temp=*tokensIterator;
                        tokensIterator++;
                        double timestamp=stoll(temp)/prvMetaData->getTimeDivider();
                        if (currentTimestamp>timestamp){
                            Message::Critical("line "+ to_string(lineNumber)+". Events are not correctly time-sorted. Current timestamp: "+ to_string(timestamp*prvMetaData->getTimeDivider())+" Previous timestamp: "+to_string(currentTimestamp*prvMetaData->getTimeDivider())+". Leaving...");
                            return;
                        }
                        currentTimestamp=timestamp;
                        map<int, string>* events=new map<int, string>();
                        for (; tokensIterator!=tokens.end();){
                            temp=*tokensIterator;
                            tokensIterator++;
                            int id=atoi(temp.c_str());
                            temp=*tokensIterator;
                            tokensIterator++;
                            events->operator [](id)=temp;
                        }
                        pajeWriter->pushEvents(cpu, app, task, thread, timestamp, events, lineNumber);
                        delete events;
                    }else if (eventType.compare(PRV_BODY_STATE)==0){
                        string temp=*tokensIterator;
                        tokensIterator++;
                        int cpu=atoi(temp.c_str());
                        temp=*tokensIterator;
                        tokensIterator++;
                        int app=atoi(temp.c_str());
                        temp=*tokensIterator;
                        tokensIterator++;
                        int task=atoi(temp.c_str());
                        temp=*tokensIterator;
                        tokensIterator++;
                        int thread=atoi(temp.c_str());
                        temp=*tokensIterator;
                        tokensIterator++;
                        double startTimestamp=stoll(temp)/prvMetaData->getTimeDivider();
                        if (currentTimestamp>startTimestamp){
                            Message::Critical("line "+ to_string(lineNumber)+". Events are not correctly time-sorted. Current timestamp: "+ to_string(startTimestamp*prvMetaData->getTimeDivider())+" Previous timestamp: "+to_string(currentTimestamp*prvMetaData->getTimeDivider())+". Leaving...");
                            return;
                        }
                        currentTimestamp=startTimestamp;
                        temp=*tokensIterator;
                        tokensIterator++;
                        double endTimestamp=stoll(temp)/prvMetaData->getTimeDivider();
                        if (endTimestamp==1.000000000){
                            Message::Info("Boogie");
                        }
                        temp=*tokensIterator;
                        pajeWriter->pushState(cpu, app, task, thread, startTimestamp, endTimestamp, temp, lineNumber);
                    }
                }
            } 
        }pajeWriter->finalize();
    }
}
