// $Id: $

/*!
 * \file PHGenIntegralv1.cc
 * \brief 
 * \author Jin Huang <jhuang@bnl.gov>
 * \version $Revision:   $
 * \date $Date: $
 */

#include "PHGenIntegralv1.h"

#include <cstdlib>
#include <iostream>
using namespace std;

PHGenIntegralv1::PHGenIntegralv1()
{
  Reset();
}

PHGenIntegralv1::~PHGenIntegralv1()
{
}

PHObject* PHGenIntegralv1::clone() const
{
  //this class is simple, use the default copy constructor for cloning
  return new PHGenIntegralv1(*this);
}

void PHGenIntegralv1::identify(ostream& os) const
{
  os << "PHGenIntegralv1::identify: "
     << "N_Generator_Accepted_Event = " << get_N_Generator_Accepted_Event() << " @ " << get_CrossSection_Generator_Accepted_Event() << " pb"
     << ", N_Processed_Event = " << get_N_Processed_Event() << " @ " << get_CrossSection_Processed_Event() << " pb"
     << ", Sum_Of_Weight = " << get_Sum_Of_Weight()
     << ", Integrated_Lumi = " << get_Integrated_Lumi() << " pb^-1"
     << ", The description for source: " << get_Description()
     << endl;
}

void PHGenIntegralv1::Reset()
{
  fNProcessedEvent = 0;
  fNGeneratorAcceptedEvent = 0;
  fIntegratedLumi = 0;
  fSumOfWeight = 0;
  fDescription = "Not set";
}

void PHGenIntegralv1::Integrate(PHObject* incoming_object)
{
  PHGenIntegral* in_gen = dynamic_cast<PHGenIntegral*>(incoming_object);

  if (!in_gen)
  {
    cout << "PHGenIntegralv1::Integrate - Fatal Error - "
         << "input object is not a PHGenIntegral: ";
    incoming_object->identify();

    exit(EXIT_FAILURE);
  }

  fNProcessedEvent += in_gen->get_N_Processed_Event();
  fNGeneratorAcceptedEvent += in_gen->get_N_Generator_Accepted_Event();
  fIntegratedLumi += in_gen->get_Integrated_Lumi();
  fSumOfWeight += in_gen->get_Sum_Of_Weight();

  if (fDescription != in_gen->get_Description())
  {
    fDescription = fDescription + ", and " + in_gen->get_Description();
  }
}
