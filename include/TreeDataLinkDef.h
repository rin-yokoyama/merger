#ifdef __CINT__

//#pragma link off all globals;
//#pragma link off all classes;
//#pragma link off all functions;

#pragma link C++ class TreeData+;
#pragma link C++ class PixTreeEvent+;
#pragma link C++ class OutputTreeData<PixTreeEvent, TreeData>+;
#pragma link C++ class OutputTreeData<PixTreeEvent, OutputTreeData<PixTreeEvent,TreeData>>+;
#pragma link C++ class TTreeReaderValue<OutputTreeData<PixTreeEvent, TreeData>>+;
#pragma link C++ class TTreeReaderValue<OutputTreeData<PixTreeEvent, OutputTreeData<PixTreeEvent,TreeData>>>+;
#pragma link C++ class AnamergerPidSelector+;

#endif

