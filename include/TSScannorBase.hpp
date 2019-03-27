/** TSScannorBaseBase.hpp generated by R. Yokoyama 09/04/2018 **/

#ifndef VANDLE_MERGER_TSSCANNORBASE_HPP_
#define VANDLE_MERGER_TSSCANNORBASE_HPP_

#include <TFile.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include "YamlReader.hpp"
#include "RemainTime.h"

/** Base class for scanning timestamp from tree **/
template <class T>
class TSScannorBase 
{
public:
    const static std::string kMsgPrefix;

    TSScannorBase();
    virtual ~TSScannorBase();

    void Configure(const std::string &yaml_node_name);
    virtual void SetReader(); // virtual function to define TTreeReaderValue
    void Scan(); // scan through events from first_entry_ to last_entry_ of the tree
    //std::map<ULong64_t,ULong64_t> GetMap(){ return ts_entry_map_;}
    std::map<ULong64_t,T> GetMap(){ return ts_entry_map_;}
    void Restart(){ tree_reader_->Restart(); }
    T* GetEntry(const ULong64_t &i_entry)
    {
        if(!tree_data_) throw kMsgPrefix + "GetEntry(), tree_data_ is null.";
        tree_reader_->SetEntry(i_entry);
        if(!tree_data_->Get()) throw kMsgPrefix + "GetEntry(), tree_data_->Get() returned null.";
        return tree_data_->Get();
    };
    ULong64_t GetCurrentEntry(){
        if(!tree_data_) throw kMsgPrefix + "GetCurrentEntry(), tree_data_ is null.";
        return tree_reader_->GetCurrentEntry();
    }
    TTree* GetTree() const { return tree_reader_->GetTree(); }
    std::map<std::string, std::pair<std::string,void*>> GetBranchMap() const { return branch_map_; }
    void SetBranchAddress();

protected:
    TFile *tree_file_; // Input tree TFile
    TTreeReader *tree_reader_; // TTreeReader
    YamlReader *yaml_reader_; // config reader
    //std::map<ULong64_t,ULong64_t> ts_entry_map_; // map of key=timestamp, value=index
    std::map<ULong64_t,T> ts_entry_map_; // map of key=timestamp, value=index
    RemainTime *remain_time_; // estimates remaining time
    ULong64_t first_entry_; // the index of the first entry to scan
    ULong64_t last_entry_; // the index of the last entry to scan
    ULong64_t print_freq_; // frequency to print scan progress
    std::map<std::string, std::pair<std::string,void*>> branch_map_; // other branches to copy

    virtual ULong64_t GetTS() const = 0;
    virtual Bool_t IsInGate() {return true;}
    TTreeReaderValue<T> *tree_data_;
};

template <class T> const std::string TSScannorBase<T>::kMsgPrefix("[TSScannorBase]:");

template <class T> TSScannorBase<T>::TSScannorBase()
{
    tree_file_ = nullptr;
    tree_reader_ = nullptr;
    yaml_reader_ = nullptr;
}

template <class T> TSScannorBase<T>::~TSScannorBase()
{
    tree_file_->Close();
    if(tree_file_)
        delete tree_file_;
    if(tree_reader_)
        delete tree_reader_;
    if(yaml_reader_)
        delete yaml_reader_;
}

template <class T> void TSScannorBase<T>::Configure(const std::string &yaml_node_name)
{
    /** loads configuration from yaml **/
    if(yaml_reader_){
        delete yaml_reader_;
        yaml_reader_ = nullptr;
    }
    yaml_reader_ = new YamlReader(yaml_node_name);
    std::string input_file_name = yaml_reader_->GetString("InputFileName");
    std::string tree_name = yaml_reader_->GetString("TreeName");

    /** opens input root file **/
    if(tree_file_){
        delete tree_file_;
        tree_file_ = nullptr;
    }
    tree_file_ = new TFile(input_file_name.c_str());
    std::cout << kMsgPrefix << "file open \"" << input_file_name << "\"" << std::endl;

    /** creates TTreeReader **/
    if(tree_reader_){
        delete tree_reader_;
        tree_reader_ = nullptr;
    }
    tree_reader_ = new TTreeReader(tree_name.c_str(),tree_file_);

    /** generates a map of branch addresses for branch outputs **/
    {
        YAML::Node doc = yaml_reader_->GetNode("OtherBranches",false);
        for(int i=0; i<doc.size(); ++i) {
            std::string name = doc[i].as<std::string>();
            TBranch* branch =(TBranch*)tree_reader_->GetTree()->FindBranch(name.c_str()); 
            std::string class_name(branch->GetClassName());
            auto na_pair = std::pair<std::string,void*>(class_name,nullptr);
            branch_map_.emplace(std::pair<std::string,std::pair<std::string,void*>>(name,na_pair));
            std::cout << kMsgPrefix << "added an output branch, " << class_name << " " << name << std::endl;
        }
    }

    /** scan entries range **/
    first_entry_ = yaml_reader_->GetULong64("FirstEntry",false,0);
    last_entry_ = yaml_reader_->GetULong64("LastEntry",false,tree_reader_->GetEntries(true));
    tree_reader_->SetEntriesRange(first_entry_, last_entry_);

    /** frequency of printing scan progress default = 10000 **/
    print_freq_ = yaml_reader_->GetULong64("PrintFrequency",false,10000);
    return;
}

template <class T> void TSScannorBase<T>::SetReader()
{
    if(!yaml_reader_){
        std::cout << kMsgPrefix << "SetReader() called before Configure()." << std::endl;
        return;
    }
}

template <class T> void TSScannorBase<T>::Scan()
{
    if(!tree_reader_)
        throw kMsgPrefix + "Scan() called but tree_reader_ is null.";
        
    const ULong64_t total_entry = last_entry_ - first_entry_;
    RemainTime remain_time(total_entry);

    while ( tree_reader_->Next() )
    {
        /** If the event is in the gate, emplace <timestamp, index> to the map **/
        if ( IsInGate() ){
            //ts_entry_map_.emplace(std::make_pair(GetTS(), tree_reader_->GetCurrentEntry()));
            ts_entry_map_.emplace(std::make_pair(GetTS(), *tree_data_->Get()));
        }
        /** displays progress **/
        ULong64_t i_entry = tree_reader_->GetCurrentEntry() - first_entry_;
        if ( !(tree_reader_->GetCurrentEntry()%print_freq_) && i_entry){
            tm *remain = remain_time.remain(i_entry);
            std::cout << kMsgPrefix << tree_reader_->GetCurrentEntry() << "/" << last_entry_ << " ";
            std::cout << 100.*(double)i_entry/(double)(total_entry) << "\% scanned. Remaining " << remain->tm_hour << "h ";
            std::cout << remain->tm_min << "m " << remain->tm_sec << "s" << std::endl;
        }
    }
    return;
}

template <class T> void TSScannorBase<T>::SetBranchAddress(){
    for(auto br: branch_map_){
        tree_reader_->GetTree()->SetBranchAddress(br.first.c_str(),br.second.second);
    }
    return;
}

#endif /* VANDLE_MERGER_TSSCANNORBASE_HPP_ */
