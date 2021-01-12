/** TreeMerger.hpp generated by R. Yokoyama 09/07/2018 **/

#ifndef VANDLE_MERGER_TREEMERGER_HPP_
#define VANDLE_MERGER_TREEMERGER_HPP_

#include <TFile.h>
#include "YamlReader.hpp"
#include "RemainTime.h"
#include "TSScannorBase.hpp"
#include "Compression.h"

/** merger class **/
// input_2 events will be merged to a vector in input_1

template <class TOUT, class TIN1, class TIN2>
class TreeMerger
{
public:
    const static std::string kMsgPrefix;

    TreeMerger();
    TreeMerger(TSScannorBase<TIN1> *input1, TSScannorBase<TIN2> *input2);
    virtual ~TreeMerger();

    void Configure(const std::string &yaml_node_name); // loads configuration from yaml
    void Merge(); // merge events from input2 to input1
    void Write(); // writes tree to the file
    virtual bool IsInGate(const TIN1 &in1, const TIN2 &in2);
    void SetFileSuffix(const std::string suffix) { file_suffix_ = suffix; }

protected:
    TFile *tree_file_; // Output tree TFile
    TTree *tree_; // Output tree
    YamlReader *yaml_reader_; // config reader
    RemainTime *remain_time_; // estimates remaining time
    Double_t time_window_low_; // lower limit of the time window to merge events (T1-T2)
    Double_t time_window_up_; // upper limit of the time window to merge events (T1-T2)
    Double_t time_offset_; // offset of the time window to merge events Tlow + Toffset < T1 - T2 < Tup + Toffset
    Double_t scan_window_; // scan window factor to load entries to meory. (see def. in Configure())
    Double_t ts_scale_; // timestamp scale
    ULong64_t print_freq_; // frequency to print scan progress
    TOUT output_object_; // output class object
    std::string file_suffix_ = ""; // suffix to the output TFile

    /** timestamp scannors of input trees **/
    TSScannorBase<TIN1> *input_scannor_1_; 
    TSScannorBase<TIN2> *input_scannor_2_;

};

template <class TOUT, class TIN1, class TIN2>
const std::string TreeMerger<TOUT,TIN1,TIN2>::kMsgPrefix("[TreeMerger]:");

template <class TOUT, class TIN1, class TIN2>
TreeMerger<TOUT,TIN1,TIN2>::TreeMerger()
{
    tree_file_ = nullptr;
    yaml_reader_ = nullptr;
    yaml_reader_ = nullptr;
    tree_ = nullptr;
    input_scannor_1_ = nullptr;
    input_scannor_2_ = nullptr;
}

template <class TOUT, class TIN1, class TIN2>
TreeMerger<TOUT,TIN1,TIN2>::TreeMerger(TSScannorBase<TIN1> *input1, TSScannorBase<TIN2> *input2)
{
    tree_file_ = nullptr;
    yaml_reader_ = nullptr;
    yaml_reader_ = nullptr;
    tree_ = nullptr;
    input_scannor_1_ = input1;
    input_scannor_2_ = input2;
}

template <class TOUT, class TIN1, class TIN2>
TreeMerger<TOUT,TIN1,TIN2>::~TreeMerger()
{
    if(tree_)
        delete tree_;
    if(tree_file_){
        tree_file_->Close();
        delete tree_file_;
    }
    if(yaml_reader_)
        delete yaml_reader_;
}

template <class TOUT, class TIN1, class TIN2>
void TreeMerger<TOUT,TIN1,TIN2>::Configure(const std::string &yaml_node_name)
{
    /** loads configuration from yaml **/
    if(yaml_reader_){
        delete yaml_reader_;
        yaml_reader_ = nullptr;
    }
    yaml_reader_ = new YamlReader(yaml_node_name);

    /** opens output root file **/
    std::string output_file_name = yaml_reader_->GetString("OutputFileName");
    output_file_name.replace(output_file_name.begin()+output_file_name.find_last_of("."), output_file_name.end(), file_suffix_ + ".root");
    std::string file_option = yaml_reader_->GetString("FileOption",false,"RECREATE");
    if(tree_file_){
        delete tree_file_;
        tree_file_ = nullptr;
    }
    tree_file_ = new TFile(output_file_name.c_str(),file_option.c_str(),"",ROOT::CompressionSettings(ROOT::kLZMA, 8));
    std::cout << kMsgPrefix << "output file open \"" << output_file_name << "\"" << std::endl;

    /** output tree **/
    std::string tree_name = yaml_reader_->GetString("TreeName");
    if(tree_){
        delete tree_;
        tree_ = nullptr;
    }
    tree_ = new TTree(tree_name.c_str(),tree_name.c_str());
    std::cout << kMsgPrefix << "output TTree \"" << tree_name << "\" created." << std::endl;

    /** output branch **/
    std::string branch_name = yaml_reader_->GetString("BranchName");
    std::string class_name = yaml_reader_->GetString("ClassName");
    Int_t buffsize = yaml_reader_->GetULong64("BuffSize",false,32000);
    Int_t splitlevel = yaml_reader_->GetULong64("SplitLevel",false,99);
    tree_->Branch(branch_name.c_str(),class_name.c_str(),&output_object_,buffsize,splitlevel);

    /** time window **/
    time_window_low_ = yaml_reader_->GetDouble("TimeWindowLow");
    time_window_up_ = yaml_reader_->GetDouble("TimeWindowUp");
    time_offset_ = yaml_reader_->GetDouble("TimeStampOffset",false,0);
    time_window_up_ = time_window_up_ + time_offset_;
    time_window_low_ = time_window_low_ + time_offset_;
    ts_scale_ = yaml_reader_->GetDouble("TimeStampScale",false,1);
    /** How many times the time window to load entries to memory at a time  **/
    /** events in a scan_window_*(time_window_low_+time_window_up_) window will be load at a time **/
    scan_window_ = yaml_reader_->GetDouble("ScanWindow",false,100);

    /** frequency of printing scan progress default = 10000 **/
    print_freq_ = yaml_reader_->GetULong64("PrintFrequency",false,10000);

    /** SetBranchAddress for other branches **/
    auto b_map = input_scannor_1_->GetBranchMap();
    for( const auto &br : b_map){
        tree_->Branch(br.first.c_str(),br.second.first.c_str(),br.second.second); 
        std::cout << kMsgPrefix << "Branch(" << br.first << ", " << br.second.first << ", " << br.second.second << ")" << std::endl;
    }
    input_scannor_1_->SetBranchAddress();
    return;
}

template <class TOUT, class TIN1, class TIN2>
void TreeMerger<TOUT,TIN1,TIN2>::Merge()
{
    std::cout << "TreeMerger::Merge()" << std::endl;
    if((!input_scannor_1_)||(!input_scannor_2_))
        throw kMsgPrefix + "Merger(), pointer to input scannors are null";
 
    auto map1 = input_scannor_1_->GetIEntryMap();
    std::pair<ULong64_t,Double_t> map_window = std::make_pair<ULong64_t,Double_t>(0,0);
 
    const ULong64_t total_entry = map1.size();
    RemainTime remain_time(total_entry); // set total number of entries to estimate remaining time.
    ULong64_t i_entry = 0;
    input_scannor_1_->Restart();
    input_scannor_2_->Restart();
    std::cout << "TreeMerger::Merge():total entry " << total_entry << std::endl;
    std::map<ULong64_t,TIN2>* map2;
    for ( auto entry :  map1 )
    {
        /** displays progress **/
        if ( !(i_entry%print_freq_) && i_entry){
            tm *remain = remain_time.remain(i_entry);
            std::cout << kMsgPrefix << i_entry << "/" << total_entry << " ";
            std::cout << 100.*(double)i_entry/(double)(total_entry) << "\% merged. Remaining " << remain->tm_hour << "h ";
            std::cout << remain->tm_min << "m " << remain->tm_sec << "s" << std::endl;
        }
        ++i_entry;

        /** loop over input2 events whithin T1-up < T2 < T1+low **/
        const ULong64_t t_low = (ULong64_t)(entry.first*ts_scale_ - time_window_up_);
        const ULong64_t t_up = (ULong64_t)(entry.first*ts_scale_ + time_window_low_);

        /** load entries to memory if needed **/
        if( map_window.first > t_low || map_window.second < t_up ){
            map_window.second = t_low + scan_window_*(time_window_up_ + time_window_low_);
            map2 = input_scannor_2_->LoadEntries(t_low, map_window.second); 
            map_window.first = t_low;
        }

        auto it = map2->lower_bound(t_low);
        auto last = map2->upper_bound(t_up);
        if( it == map2->end() || it == last ) // Skips if there is no correlated event.
            continue;

        const TIN1 *input1 = input_scannor_1_->GetEntry(entry.second);
        TOUT o_obj(*input1); // initializes output object with input1 entry

        while ( it != last )
        {
            if(IsInGate(*input1,it->second))
                o_obj.output_vec_.emplace_back(it->second);
            ++it; 
        }
        output_object_ = o_obj;
        input_scannor_1_->GetTree()->GetEntry(entry.second);
		  if(!output_object_.output_vec_.empty())
				tree_->Fill();
    }

    return;
}

template <class TOUT, class TIN1, class TIN2>
void TreeMerger<TOUT,TIN1,TIN2>::Write()
{
    /** writes to the output file **/
    std::cout << kMsgPrefix << tree_->GetEntries() << " events have been merged." << std::endl;
    std::cout << kMsgPrefix << "writing " << tree_->GetName() << " to " << tree_file_->GetName() << std::endl;
    if(!tree_file_){
        std::cout << kMsgPrefix << "Output file is not open. No tree has been written." << std::endl;
        return;
    }
    tree_file_->Write();
    return;
}

template <class TOUT, class TIN1, class TIN2>
bool TreeMerger<TOUT,TIN1,TIN2>::IsInGate(const TIN1 &in1, const TIN2 &in2)
{
    return true;
}

#endif /* VANDLE_MERGER_TREEMERGER_HPP_ */
