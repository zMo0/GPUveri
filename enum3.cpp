#include <windows.h>
#include <Setupapi.h>
#include <devguid.h>
#include <tchar.h>

#include <stdio.h>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <deque>
using namespace std;

//g++ enum3.cpp -lsetupapi -luuid -o enum3 
//-std=c++11 or later

string catfp;

string convertToString(char* a, int size)
{
    int i;
    string str = "";
    for (i = 0; i < size; i++) {
        str = str + a[i];
    }
    
    return str;
}

UINT CALLBACK callBackroutine(PVOID Context, UINT Notification, UINT_PTR Param1, UINT_PTR Param2){
    PFILEPATHS filePaths = (PFILEPATHS) Param1;
    //printf("%s\n",filePaths->Source);
    string path = filePaths->Source;

    fstream driFile;
    driFile.open("dri.txt", ios_base::out);
    driFile<< path<<endl;
    driFile.close();

    system("signtoolVeri.bat");

    //always return no_error so the scan function does not 
    //stop before going through all nodes
    return NO_ERROR;
}

int main(int argc, char ** argv) 
{
    cout<<"This program enumerates local AMD graphics devices and checks for the integrity of their drivers" << endl;
    cout<<"\nPress <ENTER> to start" << endl;
    cin.ignore();

    BOOL success;

    ofstream ofile ("output.txt");
    ofile<<"---the beginning of program output---"<<endl;
    ofile.close();

    ofstream drFile ("dri.txt");
    drFile.close();

    ofstream catfile ("cat.txt");
    catfile.close();

    //create set of display class devices
    HDEVINFO deviceInfoSet;
    GUID *guidDev = (GUID*) &GUID_DEVCLASS_DISPLAY; 
    deviceInfoSet = SetupDiGetClassDevs(guidDev, 
                                        NULL,
                                        NULL, 
                                        DIGCF_PRESENT | DIGCF_PROFILE);
    
    CHAR buffer [4000];
    DWORD buffersize =4000;
    int memberIndex = 0;

    while (true) {
        /* 
            find device information        
        */
        SP_DEVINFO_DATA deviceInfoData;
        ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        
        //enumerate all display devices in deviceInfoSet
        if (SetupDiEnumDeviceInfo(deviceInfoSet, memberIndex, &deviceInfoData) == FALSE) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                cout<<"\nNo more drivers, All done! See output.txt for complete verification output" <<endl;
                break;
            }
            else{
                cout<<"Failed to find devices" << endl;
                break;
            }
        }
        
        //Find the device instance path of each device
        DWORD nSize=0 ;
        success = SetupDiGetDeviceInstanceId (deviceInfoSet, 
                                            &deviceInfoData, 
                                            buffer, 
                                            sizeof(buffer), 
                                            &nSize);
        string instance;

        if (!success){
            cout << "Failed to find Device" << endl;
            break;
        }

        else{
            buffer [nSize] ='\0';
            instance = convertToString(buffer, nSize);
            cout<<"Device Instance: "<< buffer<<endl;
        }
        
        //Decide if the device's vendor is AMD
        string vendorID = "VEN_1002";

/*         if (instance.find(vendorID) == string::npos) {
            cout<< "Not an AMD device, continuing..."<<endl;
            memberIndex++;
            continue;
        }  */

        /* 
            Locate associated driver files and catalog files 

        */
        
        //Build a list of the device's driver packages
        success = SetupDiBuildDriverInfoList(deviceInfoSet, 
                                            &deviceInfoData, 
                                            SPDIT_COMPATDRIVER);
        if (!success){
            cout<<"Failed to build driver info list" << endl;
            break;
        } 

        SP_DRVINFO_DATA driverInfoData;
        ZeroMemory(&driverInfoData, sizeof(SP_DRVINFO_DATA));
        driverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        int driverIndex = 0;

        //enumerate drivers
        while(true){
            
            if (SetupDiEnumDriverInfo  (deviceInfoSet, &deviceInfoData, 
                                        SPDIT_COMPATDRIVER, driverIndex, 
                                        &driverInfoData) == FALSE){
                if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                    break;
                }
            }

            //find detailed info of current driver
            SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
            ZeroMemory(&DriverInfoDetailData, sizeof(SP_DRVINFO_DETAIL_DATA));
            DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
            TCHAR buffer2 [8000];
            DWORD rSize=0 ;

            success = SetupDiGetDriverInfoDetail(deviceInfoSet, 
                                                &deviceInfoData,
                                                &driverInfoData, 
                                                &DriverInfoDetailData, 
                                                sizeof(buffer2), 
                                                &rSize);
            if (!success){
                cout<<"Failed to obtain driver detail" << endl;
                break;
            } 

            cout<< "\nDriver Path: " << DriverInfoDetailData.InfFileName <<endl;
            cout<< "Checking files associated with the above driver: " << endl;
            string INFname = convertToString(&DriverInfoDetailData.InfFileName[0], 260);

            int begin = INFname.find_last_of("\\");
            int end = INFname.find_last_of(".");
            string catfile = INFname.substr(begin+1, end-begin);
            catfile.append("cat");
            cout<<"Catalog file: "<< catfile <<endl;

            /*

                locate catalog file path by traversing the C:\\Windows\\System32\\CatRoot
                directory and its top subdirectories

            */
            string path = "C:\\Windows\\System32\\CatRoot\\{F750E6C3-38EE-11D1-85E5-00C04FC295EE}\\*";
            string catPath;

            WIN32_FIND_DATA ffd;
            LARGE_INTEGER filesize;
            HANDLE hFind = INVALID_HANDLE_VALUE;
            const TCHAR * szDir = new TCHAR[MAX_PATH]; 
            szDir = path.c_str();
            bool found = FALSE;

            hFind = FindFirstFile(szDir, &ffd);

            while (FindNextFile(hFind, &ffd) != 0){
                if (ffd.cFileName == catfile){
                    catPath = szDir;
                    catPath.pop_back();
                    catPath.append(catfile);
                    found = true;
                    cout<<"Catalog file path: "<<catPath<<endl;

                    //export catalog file path
                    fstream catFile;
                    catFile.open("cat.txt", ios_base::out);
                    catFile<< catPath <<endl;
                    catFile.close();
                    catfp = catPath;

                    cout<<"\npress <Enter> to start verification" <<endl;
                    cin.ignore();
                    cout<<"Verifying, please wait...\n" <<endl;
                    break;
                }
                
                //filesize.LowPart = ffd.nFileSizeLow;
                //filesize.HighPart = ffd.nFileSizeHigh;
                //_tprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
            }
            if (!found){
                cout<< "catalog file not found"<<endl;
            }  
           
            else{
                /*
                    Find driver files and
                    Verify signature
                */
                
                //Do the file-copying part of driver installation
                success = SetupDiSetSelectedDriver(deviceInfoSet,
                                                &deviceInfoData,
                                                &driverInfoData); 
                if (!success){
                    cout<<"Failed to obtain driver file list" << endl;
                    break;
                } 

                //Declare file queue
                HSPFILEQ fileQueueHandle = SetupOpenFileQueue();
                if (INVALID_HANDLE_VALUE == fileQueueHandle){
                        cout<<"Out of memory, could not open file queue" << endl;
                        break;
                }

                //Modify driver parameters to use file queue handle provided above
                SP_DEVINSTALL_PARAMS devInstallParams;
                ZeroMemory(&devInstallParams, sizeof(SP_DEVINSTALL_PARAMS));
                devInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

                devInstallParams.FileQueue = fileQueueHandle;
                devInstallParams.Flags = DI_NOVCP;
                success = SetupDiSetDeviceInstallParams(deviceInfoSet,
                                                        &deviceInfoData,
                                                        &devInstallParams);
               if (!success){
                    cout<<"Failed to set installation parameters" << endl;
                    break;
                } 
                //queue files
                success  = SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                                    deviceInfoSet, 
                                                    &deviceInfoData);
                if (!success){
                    cout<<"Failed to create file queue" << endl;
                    break;
                } 

                //find all driver files
                DWORD scanRes;
                success = SetupScanFileQueue(fileQueueHandle,
                                    SPQ_SCAN_USE_CALLBACKEX,
                                    NULL,
                                    callBackroutine,
                                    NULL,
                                    &scanRes);
                if (!success){
                    cout<<"Failed to scan all files in queue" << endl;
                    break;
                } 

                SetupCloseFileQueue(fileQueueHandle); 
            }

            driverIndex++;
            cout<<"\nVerification complete, <Enter> to process the next driver" <<endl;
            cin.ignore();
        }     
        memberIndex++;
    }

    if (deviceInfoSet) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }

    cout<< "Press <ENTER> to exit" << endl;
    cin.ignore();

    remove ("cat.txt");
    remove("dri.txt");

    return 0;
}