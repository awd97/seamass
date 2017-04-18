//
// Author: Ranjeet Bhamber <ranjeet <a.t> bristol.ac.uk>
//
// Copyright (C) 2016  biospi Laboratory, University of Bristol, UK
//
// This file is part of seaMass.
//
// seaMass is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// seaMass is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with seaMass.  If not, see <http://www.gnu.org/licenses/>.
//

#include "DatasetMzmlb.hpp"
#include <algorithm>
#include <iomanip>
#include <map>
#include <cassert>
#include <pugixml.hpp>
#include "mzMLxml.hpp"
#include "../peak/SMData.hpp"
#include "../peak/MathOperator.hpp"
#include "../peak/BsplineData.hpp"
#include "../peak/PeakOperator.hpp"
#include "../peak/PeakData.hpp"
#include "../peak/PeakManager.hpp"
using namespace kernel;
namespace xml = pugi;


DatasetMzmlb::DatasetMzmlb(string& fileName) : spectrumIndex_(0), lastSpectrumIndex_(-1000)
{
    if (getDebugLevel() % 10 >= 1)
        cout << getTimeStamp();
    cout << "Querying " << fileName << " ..." << endl;

    file_.open(fileName);

    // Load "mzML_spectrumIndex"
    vector<li> mzML_spectrumIndex;
    file_.read_VecNC("mzML_spectrumIndex", mzML_spectrumIndex);

    // Load "mzML_chromatogramIndex"
    vector<li> mzML_chromatogramIndex;
    file_.read_VecNC("mzML_chromatogramIndex", mzML_chromatogramIndex);

    // Load "mzML" but without spectra
    vector<char> mzML;
    {
        vector<char> buffer;
        size_t offset = 0;
        size_t extent = (size_t) mzML_spectrumIndex.front();
        file_.read_HypVecNC("mzML", buffer, &offset, &extent);
        mzML.insert(mzML.end(), buffer.begin(), buffer.end());

        offset = (size_t) mzML_spectrumIndex.back();
        extent = mzML_chromatogramIndex.front() - offset;
        file_.read_HypVecNC("mzML", buffer, &offset, &extent);
        mzML.insert(mzML.end(), buffer.begin(), buffer.end());

        offset = (size_t) mzML_chromatogramIndex.back();
        vector<size_t> dims = file_.read_DimNC("mzML");
        extent = dims[0] - offset;
        file_.read_HypVecNC("mzML", buffer, &offset, &extent);
        mzML.insert(mzML.end(), buffer.begin(), buffer.end());
    }

    // parse
	xml::xml_document mzmlDoc;
	xml::xpath_node_set nodes;
    xml::xml_parse_result result = mzmlDoc.load_buffer_inplace(&mzML[0], sizeof(char) * mzML.size());
    if (!result) throw runtime_error("Error: In mzMLb input - " + string(result.description()));

    map<string, SpectrumMetadata::DataType> dataTypes;
    nodes = mzmlDoc.select_nodes("mzML/instrumentConfigurationList/instrumentConfiguration");
    for(xml::xpath_node_set::const_iterator itr = nodes.begin(); itr != nodes.end(); ++itr)
    {
        string id;
        istringstream(itr->node().attribute("id").value()) >> id;

        SpectrumMetadata::DataType dataType = SpectrumMetadata::Unknown;
        xml::xpath_node_set detectors = itr->node().select_nodes("componentList/detector");
        for(xml::xpath_node_set::const_iterator itrDetectors = detectors.begin(); itrDetectors != detectors.end(); ++itrDetectors)
        {
            // is it an "electron multiplier" detector?
            xml::xpath_node_set detectorTypes = itrDetectors->node().select_nodes("cvParam[@accession='MS:1000253']");
            if(!detectorTypes.empty())
            {
                if (getDebugLevel() % 10 >= 1)
                    cout << getTimeStamp() << " Detector: electron multiplier (ion count)" << endl;
                if (dataType == SpectrumMetadata::Unknown || dataType == SpectrumMetadata::IonCount)
                    dataType = SpectrumMetadata::IonCount;
                else
                    throw runtime_error("Error: Inconsistent <detector> types found in mzMLb");
            }
            // is it an "photomultiplier" detector?
            detectorTypes = itrDetectors->node().select_nodes("cvParam[@accession='MS:1000116']");
            if(!detectorTypes.empty())
            {
                if (getDebugLevel() % 10 >= 1)
                    cout << getTimeStamp() << " Detector: photomultiplier (ion count)" << endl;
                if (dataType == SpectrumMetadata::Unknown || dataType == SpectrumMetadata::IonCount)
                    dataType = SpectrumMetadata::IonCount;
                else
                    throw runtime_error("Error: Inconsistent <detector> types found in mzMLb");
            }
            // is it an "induction" detector (i.e. orbitrap or FT-ICR)?
            detectorTypes = itrDetectors->node().select_nodes("cvParam[@accession='MS:1000624']");
            if(!detectorTypes.empty())
            {
                if (getDebugLevel() % 10 >= 1)
                    cout << getTimeStamp() << " Detector: inductive (ion current)" << endl;
                if (dataType == SpectrumMetadata::Unknown || dataType == SpectrumMetadata::IonCurrent)
                    dataType = SpectrumMetadata::IonCurrent;
                else
                    throw runtime_error("Error: Inconsistent <detector> types found in mzMLb");
            }
        }

        dataTypes[id] = dataType;
    }

    // query default detector
    SpectrumMetadata::DataType defaultDataType = dataTypes[mzmlDoc.select_node("mzML/run").node().attribute("defaultInstrumentConfigurationRef").value()];

    // query number of spectra to create "metadata_" structure
	li ns;
	istringstream(mzmlDoc.child("mzML").child("run").child("spectrumList").attribute("count").value()) >> ns;
    metadata_.resize(ns);

    for (li i = 0; i < ns; i++)
    {
       // load each spectrum's mzML to get rest of info
        size_t offset = mzML_spectrumIndex[i];
        size_t extent = mzML_spectrumIndex[i + 1] - offset;
        file_.read_HypVecNC("mzML", mzML, &offset, &extent);

        //string str(mzML.begin(), mzML.end());
        //cout << str << endl;

        xml::xml_parse_result result = mzmlDoc.load_buffer_inplace(&mzML[0], sizeof(char) * mzML.size());
        if (!result) throw runtime_error("Error: In mzMLb input file - " + string(result.description()));

        // capture index
        istringstream(mzmlDoc.select_node("spectrum").node().attribute("index").value()) >> metadata_[i].mzmlSpectrumIndex;

        // capture mz and intensity array length
        istringstream(mzmlDoc.select_node("spectrum").node().attribute("defaultArrayLength").value()) >> metadata_[i].defaultArrayLength;

        // capture if MS1 spectrum
        nodes = mzmlDoc.select_nodes("spectrum/cvParam[@accession='MS:1000579']");
        if (!nodes.empty())
            metadata_[i].config = "MS1";

        // capture if MSn spectrum
        nodes = mzmlDoc.select_nodes("spectrum/cvParam[@accession='MS:1000580']");
        if (!nodes.empty())
            metadata_[i].config = "MSn";

        // capture if profile mode spectrum
        nodes = mzmlDoc.select_nodes("spectrum/cvParam[@accession='MS:1000128']");
        if (!nodes.empty())
            metadata_[i].isProfileMode = true;
        else
            metadata_[i].isProfileMode = false;

        // capture polarity
        nodes = mzmlDoc.select_nodes("spectrum/cvParam[@accession='MS:1000129']");
        if (!nodes.empty())
            metadata_[i].id = "n";
        else
        {
            nodes = mzmlDoc.select_nodes("spectrum/cvParam[@accession='MS:1000130']");
            if (!nodes.empty())
                metadata_[i].id = "p";
            else
                metadata_[i].id = "u";
        }

        // capture scan info (we can only process files with one scan per spectra, so for us scan = spectrum)
        nodes = mzmlDoc.select_nodes("spectrum/scanList/scan");
        if(!nodes.empty())
        {
            if (nodes.size() > 1)
                throw runtime_error("Error: We don't know how to process multiple <scan> per <spectrum> in mzMLb input file");

            // capture spectrum detector
            if (!nodes.first().node().attribute("instrumentConfigurationRef").empty())
                metadata_[i].dataType = dataTypes[nodes.first().node().attribute("instrumentConfigurationRef").value()];
            else
                metadata_[i].dataType = defaultDataType;

            // capture start time
            xml::xpath_node_set scanStartTimes = nodes.first().node().select_nodes("cvParam[@accession='MS:1000016']");
            if(!scanStartTimes.empty())
            {
                metadata_[i].startTimeString = scanStartTimes.first().node().attribute("value").value();
                istringstream(metadata_[i].startTimeString) >> metadata_[i].startTime;

                if(string(scanStartTimes.first().node().attribute("unitAccession").value()).compare("UO:0000031") == 0)
                    metadata_[i].startTime *= 60.0;
            }
            else
                metadata_[i].startTime = -1.0;

            // capture preset scan config (will tell us if sciex data is dda or dia
            xml::xpath_node_set presetScanConfigs = nodes.first().node().select_nodes("cvParam[@accession='MS:1000616']");
            if(!presetScanConfigs.empty())
            {
                metadata_[i].config += "-";
                metadata_[i].config += presetScanConfigs.first().node().attribute("value").value();
            }
        }
        else
            throw runtime_error("Error: <spectrum> missing <scan> in mzML input");

        nodes = mzmlDoc.select_nodes("spectrum/precursorList/precursor/selectedIonList/selectedIon/cvParam[@accession='MS:1000744']");
        if(!nodes.empty())
        {
            // capture precursor mz
            string precursor = nodes.first().node().attribute("value").value();
            replace(precursor.begin(), precursor.end(), '.', '-');

            metadata_[i].id += "-";
            metadata_[i].id += precursor;
        }

        // capture dataset and offset of mzs
        nodes = mzmlDoc.select_nodes("spectrum/binaryDataArrayList/binaryDataArray/cvParam[@accession='MS:1000514']/../binary");
        if(!nodes.empty())
        {
            istringstream(nodes.first().node().attribute("externalDataset").value()) >> metadata_[i].mzsDataset;
            istringstream(nodes.first().node().attribute("offset").value()) >> metadata_[i].mzsOffset;
        }
        else
            throw runtime_error("Error: No <binary> m/z data in mzMLb input file");

        // capture dataset and offset of intensities
        nodes = mzmlDoc.select_nodes("spectrum/binaryDataArrayList/binaryDataArray/cvParam[@accession='MS:1000515']/../binary");
        if(!nodes.empty())
        {
            istringstream(nodes.first().node().attribute("externalDataset").value()) >> metadata_[i].intensitiesDataset;
            istringstream(nodes.first().node().attribute("offset").value()) >> metadata_[i].intensitiesOffset;
        }
        else
            throw runtime_error("Error: No <binary> intensity data in mzMLb input file");

        if (getDebugLevel() % 10 >= 3)
        {
            cout << getTimeStamp() << "  " << metadata_[i].mzmlSpectrumIndex << " id=" << metadata_[i].id << endl;
            cout << getTimeStamp() << "    Intensities dataset=" << metadata_[i].intensitiesDataset;
            cout << " offset=" << metadata_[i].intensitiesOffset;
            cout << " extent=" << metadata_[i].defaultArrayLength;
            cout << " profile=" << (metadata_[i].isProfileMode ? "true" : "false") << endl;
            cout << getTimeStamp() << "    Mzs dataset=" << metadata_[i].mzsDataset;
            cout << " offset=" << metadata_[i].mzsOffset;
            cout << " extent=" << metadata_[i].defaultArrayLength << endl;
            //cout << " window=[" << metadata_[i].mz0 << "," << metadata_[i].mz1 << "]Th" << endl;
            cout << getTimeStamp() << "    start_time=" << metadata_[i].startTime << "s" << endl;
        }

        // display progress update
        if (getDebugLevel() % 10 >= 1)
        {
            if ((i + 1) % 10000 == 0 || (i + 1) == ns)
            {
                cout << getTimeStamp() << "  " << setw(1 + (int)(log10((float)ns))) << (i+1) << "/" << ns << endl;
            }
        }
   }

    // sort spectra into appropriate order for next()
    if (metadata_.size() > 1)
    {
        // determine start time order of spectra
        // set finish time as start time of next spectrum, must discard last spectrum as no finish time
        sort(metadata_.begin(), metadata_.end(), &DatasetMzmlb::startTimeOrder);
        for (size_t i = 0; i < metadata_.size() - 1; i++)
        {
            metadata_[i].finishTime = metadata_[i + 1].startTime;
        }
        metadata_.resize(metadata_.size() - 1);

        // remove centroided spectra as we do not process these
        vector<SpectrumMetadata>::iterator iter = metadata_.begin();
        while (iter != metadata_.end())
        {
            if (iter->isProfileMode)
            {
                ++iter;
            }
            else
            {
                // erase returns the new iterator
                iter = metadata_.erase(iter);
            }
        }

        // sort into contiguous blocks for seaMass
        sort(metadata_.begin(), metadata_.end(), &DatasetMzmlb::seamassOrder);
    }
    else
    {
        metadata_[0].finishTime = -1.0; // startTime and finishTime not used anyway for 1D seaMass
    }

    // check if spectra within each 'config' have different 'id', if so then DDA else assume DIA
    bool dia = true;
    bool done = false;
    li offset = 0;
    for (ii i = 1; i <= (ii)metadata_.size(); i++)
    {
        if (i == metadata_.size() || metadata_[i].config != metadata_[i - 1].config)
        {
            li extent = i - offset;

            bool isDia = extent > 1;
            for (ii j = offset + 1; j < offset + extent; j++)
            {
                if (metadata_[j].id != metadata_[j - 1].id)
                {
                    isDia = false;
                    break;
                }
            }

            if (!isDia)
            {
                for (ii j = offset; j < offset + extent; j++)
                {
                    replace(metadata_[j].startTimeString.begin(), metadata_[j].startTimeString.end(), '.', '-');

                    metadata_[j].id += "-";
                    metadata_[j].id += metadata_[j].startTimeString;
                }
            }

            if (getDebugLevel() % 10 >= 1)
                cout << getTimeStamp() << "  config=" << metadata_[i - 1].config << " offset=" << offset << " extent=" << extent << " type=" << (isDia ? "DIA" : "DDA") << endl;

            offset = i;
        }
    }

	// Setup and start saving mzMLb output file...
	if(mzML.size() > 0) vector<char>().swap(mzML);
    idxDataArrayOffSet_=0;
	vector<InfoGrpVar> dataSet;
	vector<double> chroMz;
	vector<fp> chroBinCounts;
	vector<char> versionID;
    file_.read_VecNC("mzML_spectrumIndex",specIdx_);
    file_.read_VecNC("chromatogram_MS_1000595_double",chroMz);
    file_.read_VecNC("chromatogram_MS_1000515_float",chroBinCounts);
    file_.search_Group("mzML");
    dataSet = file_.get_Info();
    file_.read_AttNC("version",dataSet[0].varid,versionID,dataSet[0].grpid);
    size_t loc=0;
    size_t len=specIdx_[0];
    file_.read_HypVecNC("mzML",mzML,&loc,&len);

    size_t lastdot = fileName.find_last_of(".");
    string outFileName=fileName.substr(0,lastdot)+".out.mzMLb";
    fileOut_.open(outFileName,NC_NETCDF4);
    fileOut_.write_VecNC("chromatogram_MS_1000595_double",chroMz,NC_DOUBLE);
    fileOut_.write_VecNC("chromatogram_MS_1000515_float",chroBinCounts,NC_FLOAT);
    fileOut_.write_DefHypVecNC<char>("mzML",NC_UBYTE);
    fileOut_.write_DefHypVecNC<double>("spectrum_MS_1000514_double",NC_DOUBLE);
    fileOut_.write_DefHypVecNC<float>("spectrum_MS_1000515_float",NC_FLOAT);

    fileOut_.write_CatHypVecNC("mzML",mzML);
    newSpecIdx_.push_back(mzML.size());
	fileOut_.write_AttNC("mzML","version",versionID,NC_CHAR);
    mzML.clear();

    if (getDebugLevel() % 10 >= 1)
        cout << getTimeStamp() << " ";
    cout << "Processing ..." << endl;
}


DatasetMzmlb::~DatasetMzmlb()
{
}


bool DatasetMzmlb::read(Seamass::Input &out, std::string &id)
{
    // display total progress update
    if(spectrumIndex_ / 1000 > lastSpectrumIndex_ / 1000 || spectrumIndex_ >= metadata_.size())
    {
        if (getDebugLevel() % 10 == 0)
            for (ii i = 0; i < 256; i++) cout << "\b";
        else
            cout << getTimeStamp() << " ";
        cout << setw(1 + (int)(log10((float)metadata_.size()))) << spectrumIndex_ << "/" << metadata_.size() << " " << flush;
        if (getDebugLevel() % 10 >= 1 || spectrumIndex_ >= metadata_.size())
            cout << endl;
        lastSpectrumIndex_ = spectrumIndex_;
    }

    // clear out
    vector<li>().swap(out.countsIndex);
    vector<double>().swap(out.startTimes);
    vector<double>().swap(out.finishTimes);
    vector<fp>().swap(out.exposures);
    vector<fp>().swap(out.counts);
    vector<double>().swap(out.locations);

	if (spectrumIndex_ >= metadata_.size()) return false;

    // determine next set of spectra
    id = "b_" + metadata_[spectrumIndex_].id;
    bool done = false;
    li offset = spectrumIndex_;
    for (; !done; spectrumIndex_++)
    {
        // if at the end or any metadata is different in the next spectrum, we have come to the end of this channel
        if (spectrumIndex_ == metadata_.size() - 1 || metadata_[spectrumIndex_].id != metadata_[spectrumIndex_ + 1].id)
        {
            out.countsIndex.push_back((li)out.counts.size());
            done = true;
        }
    }
    extent_ = spectrumIndex_ - offset;

    // read mzs
    if ((extent_ > 1 && getDebugLevel() % 10 >= 1) || getDebugLevel() % 10 >= 2)
        cout << getTimeStamp() << "  Reading m/z values for id=" << id << " ..." << endl;
    out.startTimes.resize(extent_);
    out.finishTimes.resize(extent_);
    vector< vector<double> > mzs(extent_);
	for (li i = 0; i < extent_; i++)
	{
        out.startTimes[i] = metadata_[offset + i].startTime;
        out.finishTimes[i] = metadata_[offset + i].finishTime;

        if(metadata_[offset + i].defaultArrayLength > 0)
        {
            size_t rdLen = metadata_[offset + i].defaultArrayLength;
            size_t hypIdx = metadata_[offset + i].mzsOffset;
            file_.read_HypVecNC(metadata_[offset + i].mzsDataset, mzs[i], &hypIdx, &rdLen);
        }

        // display progress update
        if (extent_ > 1 && ((i + 1) % 1000 == 0 || (i + 1) == extent_))
        {
            if (extent_ > 1 && getDebugLevel() % 10 == 0)
                cout << "." << flush;
            else if (getDebugLevel() % 10 >= 1)
                cout << getTimeStamp() << "   " << setw(1 + (int)(log10((float)extent_))) << (i+1) << "/" << extent_ << endl;
        }
    }

    // estimate of mz_range [mz0,mz1] as
    // mzML spec does not guarentee this will exist (and hence do not know if regions have zero counts or were not scanned)
    if (extent_ > 1 && getDebugLevel() % 10 == 0)
        cout << "." << flush;
    else if ((extent_ > 1 && getDebugLevel() % 10 >= 1) || getDebugLevel() % 10 >= 2)
        cout << getTimeStamp() << "  Converting to bins ..." << endl;
    double mz0 = numeric_limits<double>::max();
    double mz1 = 0.0;
    for (ii i = 0; i < extent_; i++)
    {
        if (mzs[i].size() >= 2)
        {
            mz0 = mz0 < mzs[i].front() ? mz0 : mzs[i].front();
            mz1 = mz1 > mzs[i].back() ? mz1 : mzs[i].back();
        }
     }
    if (extent_ > 1 && getDebugLevel() % 10 >= 1)
    {
        cout << getTimeStamp() << "   autodetected_mz_window=[" << fixed << setprecision(3) << mz0 << "," << mz1 << "]Th" << endl;
    }
    // if this happens regularly, will have to use reported scan window or command line arguments
    if (mz1 <= mz0 || mz0 == numeric_limits<double>::max() || mz0 == 0.0)
        throw runtime_error("Error: Not enough datapoints in input mzMLb");

    // This modifies the raw data for some limitations of the mzML spec and makes
    // sure the intensities are treated as binned between m/z datapoints.
    //
    // For IonCount data, it interpolates the mzs to represent the edges of the bins rather
    //   than the centres
    // For IonCurrent data, it converts the sampled data to binned counts by treating the
    //   mz values as the bin edges, and using trapezoid rule to integrate intensity values
    //
    out.countsIndex.resize(extent_ + 1);
    out.exposures.resize(extent_);

    for (ii i = 0; i < extent_; i++)
    {
        out.countsIndex[i] = (li) out.counts.size();

        // load intensities
        vector<fp> intensities;
        if(metadata_[offset + i].defaultArrayLength > 0)
        {
            size_t rdLen = metadata_[offset + i].defaultArrayLength;
            size_t hypIdx = metadata_[offset + i].intensitiesOffset;
            file_.read_HypVecNC(metadata_[offset + i].intensitiesDataset, intensities, &hypIdx, &rdLen);
        }

        switch (metadata_[offset + i].dataType) {
            case SpectrumMetadata::Unknown:
            case SpectrumMetadata::IonCount: // ToF, Quad, Ion trap etc
            {
                if (intensities.size() == 0) // if empty we MUST add a zero bin!
                {
                    out.locations.push_back(mz0);
                    out.counts.push_back(0.0);
                    out.locations.push_back(mz1);

                    out.exposures[i] = 1.0;
                }
                else
                {
                    // dividing by minimum to get back to ion counts for SWATH data which appears to be automatic gain
                    // controlled to correct for dynamic range restrictions (hack!)
                    double minimum = std::numeric_limits<double>::max();
                    for (size_t k = 0; k < mzs[i].size(); k++)
                        if (intensities[k] > 0.0)
                            minimum = minimum < intensities[k] ? minimum : intensities[k];
                    // check to see if we can estimate the exposure i.e. is the minimum reasonable?
                    if (minimum < 1.0 || minimum > 1000.0) minimum = 1.0;
                    out.exposures[i] = (fp) (1.0 / minimum);

                    if (intensities.front() == 0.0) // only use the first m/z if the intensity is zero
                    {
                        double frontEdge = mz0 < mzs[i].front() ? mz0 : mzs[i].front();
                        out.locations.push_back(frontEdge);
                        out.counts.push_back(0.0);
                    }

                    // use all the intensities except first and last
                    for (ii k = 1; k < (ii) intensities.size() - 1; k++)
                    {
                        if (intensities[k] != 0.0 || intensities[k - 1] != 0.0) // merge zeros
                        {
                            out.locations.push_back(0.5 * (mzs[i][k - 1] + mzs[i][k]));
                            out.counts.push_back(((fp) intensities[k]) * out.exposures[i]);
                        }
                    }
                    out.locations.push_back(0.5 * (mzs[i][mzs[i].size() - 2] + mzs[i].back()));

                    if (intensities.back() == 0.0) // only use the last m/z if the intensity is zero
                    {
                        out.counts.push_back(0.0);
                        double backEdge = mz1 > mzs[i].back() ? mz1 : mzs[i].back();
                        out.locations.push_back(backEdge);
                    }
                }
            } break;
            case SpectrumMetadata::IonCurrent: // Orbitrap, FT-ICR etc
            {
                if (intensities.front() == 0.0 && mz0 < mzs[i].front())
                {
                    out.locations.push_back(mz0);
                    out.counts.push_back(0.0);
                }

                for (ii k = 0; k < (ii) intensities.size() - 1; k++)
                {
                    out.locations.push_back(mzs[i][k]);
                    out.counts.push_back((fp) ((mzs[i][k + 1] - mzs[i][k]) * 0.5 * (intensities[k + 1] + intensities[k])));
                }
                out.locations.push_back(mzs[i][mzs[i].size() - 1]);

                if (intensities.back() == 0.0 && mz1 > mzs[i].back())
                {
                    out.counts.push_back(0.0);
                    out.locations.push_back(mz1);
                }

                out.exposures[i] = 1.0;
            } break;
        }

        vector<double>().swap(mzs[i]);

        // display progress update
        if (extent_ > 1 && ((i + 1) % 1000 == 0 || (i + 1) == extent_))
        {
            if (extent_ > 1 && getDebugLevel() % 10 == 0)
                cout << "." << flush;
            else if (getDebugLevel() % 10 >= 1)
                cout << getTimeStamp() << "   " << setw(1 + (int)(log10((float)extent_))) << (i+1) << "/" << extent_ << endl;
        }
    }
    out.countsIndex.back() = (li)out.counts.size();

    out.type = Seamass::Input::Type::Binned;

    return true;
}


void DatasetMzmlb::write(const Seamass::Input& input, const Seamass::Output& output, const std::string& id)
{
    throw runtime_error("not implemented!");
}


bool DatasetMzmlb::startTimeOrder(const SpectrumMetadata &lhs, const SpectrumMetadata &rhs)
{
	return lhs.startTime < rhs.startTime;
}


bool DatasetMzmlb::seamassOrder(const SpectrumMetadata &lhs, const SpectrumMetadata &rhs)
{
    if (lhs.config == rhs.config)
    {
        if (lhs.id == rhs.id)
        {
            return lhs.startTime < rhs.startTime;
        }
        else
        {
            return lhs.id < rhs.id;
        }
    }
    else
    {
        return lhs.config < rhs.config;
    }
}


void DatasetMzmlb::writeData(Seamass& sm_, Seamass::Input& input_, string id, bool centriod_, double threshold_)
{
    if ((extent_ > 1 && getDebugLevel() % 10 >= 1) || getDebugLevel() % 10 >= 2)
        cout << getTimeStamp() << "  Writing=" << id << " ..." << endl;

    if(centriod_ == true)
	{
		VecMat<double> mzPeak;
		VecMat<float> pkPeak;
		vector<size_t> mzpkVecSize;

		Seamass::ControlPoints contpts;
		sm_.getOutputControlPoints1d(contpts);
		uli dims[2];
		double mzRes;
		double rtRes;
		vector<ii> offset=contpts.offset;
		mzRes=double(contpts.scale[0]);
		if(contpts.scale.size() > 1)
			rtRes=double(contpts.scale[1]);
		else
			rtRes=0;

		VecMat<float> rawCoeff(contpts.extent[1],contpts.extent[0],contpts.coeffs);
		rawCoeff.getDims(dims);

		SMData2D<OpUnit> A(dims,&offset[0],mzRes,rtRes,rawCoeff.v);
		SMData2D<OpNablaH> dhA(dims,&offset[0],mzRes,rtRes,rawCoeff.v);
		SMData2D<OpNabla2H> d2hA(dims,&offset[0],mzRes,rtRes,rawCoeff.v);

		for(size_t i = 0; i < A.rt.size(); ++i)
		{
			A.rt[i] = metadata_[i].startTime;
			dhA.rt[i] = metadata_[i].startTime;
			d2hA.rt[i] = metadata_[i].startTime;
		}

		BsplineData<> bsData(A,dhA,d2hA);

		PeakManager<PeakData,BsplineData,Centroid2D> centriodPeak(bsData,threshold_);
		centriodPeak.execute();
		centriodPeak.peak->getPeakMat(mzPeak, pkPeak, contpts.extent[1], mzpkVecSize);

		writePeakData(mzPeak,pkPeak,mzpkVecSize);
		writePeakXmlData(mzpkVecSize);

		//if(debug) centriodPeak.peak->dumpPeakData('seamassPeakPckDump.smp", NC_FLOAT);
	}
	else
	{
		vector<fp> binCounts(input_.counts.size());
		sm_.getOutputBinCounts(binCounts); // retrieve seaMass processed counts
		// convert ion counts into ion density (counts per Th) and scale by exposures
    	if (input_.exposures.size() > 0)
		{
    		if (input_.countsIndex.size() > 0)
    		{
    			// 2D data
    			for (li j = 0; j < (li)input_.countsIndex.size() - 1; j++)
    			{
    				for (li i = input_.countsIndex[j]; i < input_.countsIndex[j + 1]; i++)
       	         	{
						binCounts[i] /= (fp) (input_.locations[i + j + 1] - input_.locations[i + j]) * input_.exposures[j];
					}
				}
			}
			else
			{
				// 1D data
				for (li i = 0; i < (li)input_.counts.size(); i++)
				{
					binCounts[i] /= (fp) (input_.locations[i + 1] - input_.locations[i]) * input_.exposures[0];
				}
			}
		}
		writeVecData(binCounts); // write to mzMLb
		writeXmlData();
	}
}

void DatasetMzmlb::writeVecData(vector<float>& data_)
{
	fileOut_.write_CatHypVecNC("spectrum_MS_1000515_float",data_);
}

void DatasetMzmlb::writeXmlData()
{
	vector<char> mzML;

	if(spectrumIndex_ < metadata_.size() || spectrumIndex_ == 1)
	{
		li offset=spectrumIndex_ - extent_;
		for(size_t i = size_t(offset); i < spectrumIndex_; ++i)
		{
			if(mzML.size() > 0) vector<char>().swap(mzML);

			xml::xml_document mzmlXmlScan;
			vector<double> mzScan;
			size_t idx = metadata_[i].mzmlSpectrumIndex;
			size_t loc = size_t(specIdx_[idx]);
			size_t len = size_t(specIdx_[idx + 1] - specIdx_[idx]);
			file_.read_HypVecNC("mzML",mzML,&loc,&len);

			size_t xmlSize = sizeof(char) * mzML.size();
			xml::xml_parse_result result = mzmlXmlScan.load_buffer_inplace(&mzML[0],xmlSize);

			size_t index = getXmlValue<size_t>(mzmlXmlScan,"spectrum","index");
			size_t arrayLen = getXmlValue<size_t>(mzmlXmlScan,"spectrum","defaultArrayLength");
			size_t mzOffSet = getXmlValue<size_t>(mzmlXmlScan,
												  "spectrum/binaryDataArrayList/binaryDataArray/binary[@externalDataset='spectrum_MS_1000514_double']",
												  "offset");

			file_.read_HypVecNC("spectrum_MS_1000514_double", mzScan, &mzOffSet,
								&arrayLen);

			mzScan.erase(mzScan.begin());
			mzScan.erase(mzScan.end() - 1);

			arrayLen = mzScan.size();

			setXmlValue<size_t>(mzmlXmlScan, "spectrum", "index", i);
			setXmlValue<size_t>(mzmlXmlScan, "spectrum", "defaultArrayLength", arrayLen);

			setXmlValue<size_t>(mzmlXmlScan,
								"spectrum/binaryDataArrayList/binaryDataArray/binary[@externalDataset='spectrum_MS_1000514_double']",
								"offset", idxDataArrayOffSet_);
			setXmlValue<size_t>(mzmlXmlScan,
								"spectrum/binaryDataArrayList/binaryDataArray/binary[@externalDataset='spectrum_MS_1000515_float']",
								"offset", idxDataArrayOffSet_);
			idxDataArrayOffSet_ += arrayLen;

			stringstream newmzML;
			mzmlXmlScan.print(newmzML);
			string output = newmzML.str();
			vector<char>().swap(mzML);
			mzML.assign(output.begin(), output.end());

			newSpecIdx_.push_back(newSpecIdx_[i] + mzML.size());
			fileOut_.write_CatHypVecNC("mzML", mzML);
			fileOut_.write_CatHypVecNC("spectrum_MS_1000514_double", mzScan);
		}
	}
	if(spectrumIndex_ >= metadata_.size())
	{
		writeChromatogramXmlEnd();
	}
}

void DatasetMzmlb::writePeakData(VecMat<double>& mzPeak_, VecMat<float>& pkPeak_,
								 vector<size_t>& mzpkVecSize_)
{
	for(size_t i = 0; i < mzpkVecSize_.size(); ++i)
	{
		fileOut_.write_CatHypVecNC("spectrum_MS_1000514_double",mzPeak_.m[i],mzpkVecSize_[i]);
		fileOut_.write_CatHypVecNC("spectrum_MS_1000515_float",pkPeak_.m[i],mzpkVecSize_[i]);
	}
}

void DatasetMzmlb::writePeakXmlData(vector<size_t>& mzpkVecSize_)
{
	vector<char> mzML;

	if(spectrumIndex_ < metadata_.size() || spectrumIndex_ == 1)
	{
		li offset=spectrumIndex_ - extent_;
		size_t j=0;
		for(size_t i = size_t(offset); i < spectrumIndex_; ++i, ++j)
		{
			if(mzML.size() > 0) vector<char>().swap(mzML);

			xml::xml_document mzmlXmlScan;
			size_t idx = metadata_[i].mzmlSpectrumIndex;
			size_t loc = size_t(specIdx_[idx]);
			size_t len = size_t(specIdx_[idx + 1] - specIdx_[idx]);
			file_.read_HypVecNC("mzML",mzML,&loc,&len);

			size_t xmlSize = sizeof(char) * mzML.size();
			xml::xml_parse_result result = mzmlXmlScan.load_buffer_inplace(&mzML[0],xmlSize);

			size_t arrayLen = mzpkVecSize_[j];

			setXmlValue<size_t>(mzmlXmlScan, "spectrum", "index", i);
			setXmlValue<size_t>(mzmlXmlScan, "spectrum", "defaultArrayLength", arrayLen);

			setXmlValue<size_t>(mzmlXmlScan,
								"spectrum/binaryDataArrayList/binaryDataArray/binary[@externalDataset='spectrum_MS_1000514_double']",
								"offset", idxDataArrayOffSet_);
			setXmlValue<size_t>(mzmlXmlScan,
								"spectrum/binaryDataArrayList/binaryDataArray/binary[@externalDataset='spectrum_MS_1000515_float']",
								"offset", idxDataArrayOffSet_);
			idxDataArrayOffSet_ += arrayLen;

			stringstream newmzML;
			mzmlXmlScan.print(newmzML);
			string output = newmzML.str();
			vector<char>().swap(mzML);
			mzML.assign(output.begin(), output.end());

			newSpecIdx_.push_back(newSpecIdx_[i] + mzML.size());
			fileOut_.write_CatHypVecNC("mzML", mzML);
		}
	}
	if(spectrumIndex_ >= metadata_.size())
	{
		writeChromatogramXmlEnd();
	}
}

void DatasetMzmlb::writeChromatogramXmlEnd(){
	vector<char> mzML;
	string subxml("</spectrumList>\n");

	mzML.assign(subxml.begin(), subxml.end());
	fileOut_.write_CatHypVecNC("mzML", mzML);

	vector<size_t> lenTotal = file_.read_DimNC("mzML");
	size_t loc = specIdx_.back();
	size_t len = lenTotal[0] - loc;
	file_.read_HypVecNC("mzML", mzML, &loc, &len);

	subxml.clear();
	subxml.assign(mzML.begin(), mzML.end());
	subxml = subxml.substr(subxml.find("<chromatogramList"));

	mzML.clear();
	mzML.assign(subxml.begin(), subxml.end());

	vector<uli> newChroIdx_;
	vector<size_t> pos;
	findVecString(mzML, pos, "<chromatogram ", "</chromatogram>");
	lenTotal.clear();
	lenTotal = fileOut_.read_DimNC("mzML");
	newChroIdx_.push_back(lenTotal[0] + pos[0]);
	newChroIdx_.push_back(lenTotal[0] + pos[1]);

	fileOut_.write_CatHypVecNC("mzML", mzML);

	fileOut_.write_VecNC("mzML_spectrumIndex", newSpecIdx_, NC_INT64);
	fileOut_.write_VecNC("mzML_chromatogramIndex", newChroIdx_, NC_INT64);
}

