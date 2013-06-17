/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include <usConfig.h>

#include <usTestingMacros.h>

#include <usModule.h>
#include <usModuleContext.h>
#include <usGetModuleContext.h>
#include <usServiceInterface.h>
#include <usServiceTracker.h>

#include "usServiceControlInterface.h"
#include "usTestUtilSharedLibrary.h"

#include <memory>

US_USE_NAMESPACE

bool CheckConvertibility(const std::vector<ServiceReferenceU>& refs,
                         std::vector<std::string>::const_iterator idBegin,
                         std::vector<std::string>::const_iterator idEnd)
{
  std::vector<std::string> ids;
  ids.assign(idBegin, idEnd);

  for (std::vector<ServiceReferenceU>::const_iterator sri = refs.begin();
       sri != refs.end(); ++sri)
  {
    for (std::vector<std::string>::iterator idIter = ids.begin();
         idIter != ids.end(); ++idIter)
    {
      if (sri->IsConvertibleTo(*idIter))
      {
        ids.erase(idIter);
        break;
      }
    }
  }

  return ids.empty();
}

int usServiceTrackerTest(int /*argc*/, char* /*argv*/[])
{
  US_TEST_BEGIN("ServiceTrackerTest")

  ModuleContext* mc = GetModuleContext();
  SharedLibraryHandle libS("TestModuleS");

  // Start the test target to get a service published.
  try
  {
    libS.Load();
  }
  catch (const std::exception& e)
  {
    US_TEST_FAILED_MSG( << "Failed to load module, got exception: " << e.what() );
  }

  // 1. Create a ServiceTracker with ServiceTrackerCustomizer == null

  std::string s1("org.cppmicroservices.TestModuleSService");
  ServiceReferenceU servref = mc->GetServiceReference(s1 + "0");

  US_TEST_CONDITION_REQUIRED(servref != 0, "Test if registered service of id org.cppmicroservices.TestModuleSService0");

  ServiceReference<ServiceControlInterface> servCtrlRef = mc->GetServiceReference<ServiceControlInterface>();
  US_TEST_CONDITION_REQUIRED(servCtrlRef != 0, "Test if constrol service was registered");

  ServiceControlInterface* serviceController = mc->GetService(servCtrlRef);
  US_TEST_CONDITION_REQUIRED(serviceController != 0, "Test valid service controller");

  std::auto_ptr<ServiceTracker<void> > st1(new ServiceTracker<void>(mc, servref));

  // 2. Check the size method with an unopened service tracker

  US_TEST_CONDITION_REQUIRED(st1->Size() == 0, "Test if size == 0");

  // 3. Open the service tracker and see what it finds,
  // expect to find one instance of the implementation,
  // "org.cppmicroservices.TestModuleSService0"

  st1->Open();
  std::vector<ServiceReferenceU> sa2;
  st1->GetServiceReferences(sa2);

  US_TEST_CONDITION_REQUIRED(sa2.size() == 1, "Checking ServiceTracker size");
  US_TEST_CONDITION_REQUIRED(s1 + "0" == sa2[0].GetInterfaceId(), "Checking service implementation name");

  // 5. Close this service tracker
  st1->Close();

  // 6. Check the size method, now when the servicetracker is closed
  US_TEST_CONDITION_REQUIRED(st1->Size() == 0, "Checking ServiceTracker size");

  // 7. Check if we still track anything , we should get null
  sa2.clear();
  st1->GetServiceReferences(sa2);
  US_TEST_CONDITION_REQUIRED(sa2.empty(), "Checking ServiceTracker size");

  // 8. A new Servicetracker, this time with a filter for the object
  std::string fs = std::string("(") + ServiceConstants::OBJECTCLASS() + "=" + s1 + "*" + ")";
  LDAPFilter f1(fs);
  st1.reset(new ServiceTracker<void>(mc, f1));
  // add a service
  serviceController->ServiceControl(1, "register", 7);

  // 9. Open the service tracker and see what it finds,
  // expect to find two instances of references to
  // "org.cppmicroservices.TestModuleSService*"
  // i.e. they refer to the same piece of code

  std::vector<std::string> ids;
  ids.push_back((s1 + "0"));
  ids.push_back((s1 + "1"));
  ids.push_back((s1 + "2"));
  ids.push_back((s1 + "3"));

  st1->Open();
  sa2.clear();
  st1->GetServiceReferences(sa2);
  US_TEST_CONDITION_REQUIRED(sa2.size() == 2, "Checking service reference count");
  US_TEST_CONDITION_REQUIRED(CheckConvertibility(sa2, ids.begin(), ids.begin()+2), "Check for expected interface id [0]");
  US_TEST_CONDITION_REQUIRED(sa2[1].IsConvertibleTo(s1 + "1"), "Check for expected interface id [1]");

  // 10. Get libTestModuleS to register one more service and see if it appears
  serviceController->ServiceControl(2, "register", 1);
  sa2.clear();
  st1->GetServiceReferences(sa2);

  US_TEST_CONDITION_REQUIRED(sa2.size() == 3, "Checking service reference count");

  US_TEST_CONDITION_REQUIRED(CheckConvertibility(sa2, ids.begin(), ids.begin()+3), "Check for expected interface id [2]");

  // 11. Get libTestModuleS to register one more service and see if it appears
  serviceController->ServiceControl(3, "register", 2);
  sa2.clear();
  st1->GetServiceReferences(sa2);
  US_TEST_CONDITION_REQUIRED(sa2.size() == 4, "Checking service reference count");
  US_TEST_CONDITION_REQUIRED(CheckConvertibility(sa2, ids.begin(), ids.end()), "Check for expected interface id [3]");

  // 12. Get libTestModuleS to unregister one service and see if it disappears
  serviceController->ServiceControl(3, "unregister", 0);
  sa2.clear();
  st1->GetServiceReferences(sa2);
  US_TEST_CONDITION_REQUIRED(sa2.size() == 3, "Checking service reference count");

  // 13. Get the highest ranking service reference, it should have ranking 7
  ServiceReferenceU h1 = st1->GetServiceReference();
  int rank = any_cast<int>(h1.GetProperty(ServiceConstants::SERVICE_RANKING()));
  US_TEST_CONDITION_REQUIRED(rank == 7, "Check service rank");

  // 14. Get the service of the highest ranked service reference

  void* o1 = st1->GetService(h1);
  US_TEST_CONDITION_REQUIRED(o1 != 0, "Check for non-null service");

  // 14a Get the highest ranked service, directly this time
  void* o3 = st1->GetService();
  US_TEST_CONDITION_REQUIRED(o3 != 0, "Check for non-null service");
  US_TEST_CONDITION_REQUIRED(o1 == o3, "Check for equal service instances");

  // 15. Now release the tracking of that service and then try to get it
  //     from the servicetracker, which should yield a null object
  serviceController->ServiceControl(1, "unregister", 7);
  void* o2 = st1->GetService(h1);
  US_TEST_CONDITION_REQUIRED(o2 == 0, "Checkt that service is null");

  // 16. Get all service objects this tracker tracks, it should be 2
  std::vector<void*> ts1;
  st1->GetServices(ts1);
  US_TEST_CONDITION_REQUIRED(ts1.size() == 2, "Check service count");

  // 17. Test the remove method.
  //     First register another service, then remove it being tracked
  serviceController->ServiceControl(1, "register", 7);
  h1 = st1->GetServiceReference();
  std::vector<ServiceReferenceU> sa3;
  st1->GetServiceReferences(sa3);
  US_TEST_CONDITION_REQUIRED(sa3.size() == 3, "Check service reference count");
  US_TEST_CONDITION_REQUIRED(CheckConvertibility(sa3, ids.begin(), ids.begin()+3), "Check for expected interface id [0]");

  st1->Remove(h1);           // remove tracking on one servref
  sa2.clear();
  st1->GetServiceReferences(sa2);
  US_TEST_CONDITION_REQUIRED(sa2.size() == 2, "Check service reference count");

  // 18. Test the addingService method,add a service reference

  // 19. Test the removedService method, remove a service reference


  // 20. Test the waitForService method
  void* o9 = st1->WaitForService(50);
  US_TEST_CONDITION_REQUIRED(o9 != 0, "Checking WaitForService method");

  US_TEST_END()
}
