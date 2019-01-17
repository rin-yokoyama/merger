/** ImplantTSScannor.cpp generated by R. Yokoyama 09/06/2018 **/
#include "ImplantTSScannor.hpp"

const std::string ImplantTSScannor::kMsgPrefix("[ImplantTSScannor]:");

void ImplantTSScannor::SetReader()
{
    TSScannorBase<PspmtAnalyzerData>::SetReader();
    std::string br_name = yaml_reader_->GetString("PixieBranchName");
    tree_data_ = new TTreeReaderValue<PspmtAnalyzerData>(*tree_reader_,br_name.c_str());
    std::cout << kMsgPrefix << "TTreeReaderValue: " << br_name << " created." << std::endl;

    low_gain_min_ = yaml_reader_->GetDouble("MinLowGainDynEnergy");
    low_gain_max_ = yaml_reader_->GetDouble("MaxLowGainDynEnergy");
    std::cout << kMsgPrefix << "Implant range on dynode low gain: " << low_gain_min_ << " - " << low_gain_max_ << std::endl;
    return;
}

Bool_t ImplantTSScannor::IsInGate() const
{
    /*
    auto pspmt_low = tree_data_->Get()->low_gain_;
    auto pspmt_high = tree_data_->Get()->high_gain_;
    if(pspmt_low.xa_<10 || pspmt_low.xb_<10 || pspmt_low.ya_<10 || pspmt_low.yb_<10 )
        return false;
    if(pspmt_low.xa_>30000 || pspmt_low.xb_>30000 || pspmt_low.ya_>30000 || pspmt_low.yb_>30000)
        return false;
    if(pspmt_low.dynode_<low_gain_min_ || pspmt_low.dynode_>low_gain_max_ )
        return false;
        */
    return true;
}

const std::string MergedImplantTSScannor::kMsgPrefix("[MergedImplantTSScannor]:");

void MergedImplantTSScannor::SetReader()
{
    TSScannorBase<OutputTreeData<PspmtAnalyzerData, TreeData>>::SetReader();
    std::string br_name = yaml_reader_->GetString("PixieBranchName");
    tree_data_ = new TTreeReaderValue<OutputTreeData<PspmtAnalyzerData, TreeData>>(*tree_reader_,br_name.c_str());
    std::cout << kMsgPrefix << "TTreeReaderValue: " << br_name << " created." << std::endl;

    return;
}
