#include <Acts/Definitions/Units.hpp>
#include <Acts/EventData/MultiTrajectory.hpp>

#include <Acts/EventData/Measurement.hpp>
#include <Acts/EventData/MeasurementHelpers.hpp>


struct ResidualOutlierFinder {
  int verbosity = 0;
  std::vector<std::pair<long unsigned int, double>> chi2Cuts;

 
  bool operator()(Acts::MultiTrajectory::ConstTrackStateProxy state) const {
    // can't determine an outlier w/o a measurement or predicted parameters
    if (!state.hasCalibrated() || !state.hasPredicted()) {
      return false;
    }

    const auto predicted = state.predicted();
    const auto predictedCovariance = state.predictedCovariance();
    double chi2 = std::numeric_limits<double>::max();
    
    Acts::visit_measurement(state.calibrated(), state.calibratedCovariance(),
			    state.calibratedSize(), 
          [&](const auto calibrated, 
	      const auto calibratedCovariance) {
	        constexpr size_t kMeasurementSize = decltype(calibrated)::RowsAtCompileTime;
			
		using ParametersVector = Acts::ActsVector<kMeasurementSize>;
		const auto H = state.projector().template topLeftCorner<kMeasurementSize, Acts::eBoundSize>().eval();
		ParametersVector res;
		res = calibrated - H * predicted;
		chi2 = (res.transpose() * ((calibratedCovariance + H * predictedCovariance * H.transpose())).inverse() * res).eval()(0,0);
        
	        }); /// end lambda and call to visit meas
			    
    if(verbosity > 2)
      {
	const auto distance = (state.calibrated() - state.projector() * state.predicted()).norm();
	std::cout << "Measurement has distance, chi2 "
		  << distance << ", " << chi2 
		  << std::endl;	
      }

    auto volume = state.referenceSurface().geometryId().volume();
    auto layer = state.referenceSurface().geometryId().layer();


    bool outlier = false;
    for(auto& [vol, chi2cut] : chi2Cuts) 
      {
	if(verbosity > -1)
	  {
	    std::cout << "Meas vol id and layer " << volume << ", " << layer 
		      << " checking for volume " << vol << " and chi2cut "
		      << chi2cut << std::endl;
	  
	  }

	if(volume == vol and chi2 > chi2cut)
	  {
	    outlier = true;
	  }
      }

    return outlier;
  
  }
};





