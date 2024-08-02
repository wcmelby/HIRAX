
import time
from multiprocessing import Process, Manager
from oceandirect.OceanDirectAPI import OceanDirectAPI, OceanDirectError, Spectrometer

def read_all_serial_numbers() -> list[str]:
    """
    Read all devices serial number. Later on we assign one serial number for each
    process.
    """
    odapi = OceanDirectAPI()
    try:
        device_count = odapi.find_usb_devices()
        serialNumberList = []
        if device_count > 0:
            device_ids = odapi.get_device_ids()
            for devId in device_ids:
                device = odapi.open_device(devId)
                serialNumberList.append(device.get_serial_number())
                device.close_device()
        odapi.shutdown()
    except OceanDirectError as err:
        [errorCode, errorMsg] = err.get_error_details()
        #print("read_all_serial_numbers(): exception / error / %s / %d = %s" %
        #     (serialNumber, errorCode, errorMsg))
    return serialNumberList

# def readSpectra(serialNumber: str, integrationTimeUs: int, spectraToRead: int, output: list[float]) -> None:
def readSpectra(serialNumber: str, integrationTimeUs: int, spectraToRead: int) -> None:
    """
    This function will be executed in a separate process. This will open the device that matched the 
    given serial number.
    """
    output = []
    odapi = OceanDirectAPI()
    deviceCount = odapi.find_usb_devices()
    print('Device count:', deviceCount)
    if deviceCount > 0:
        deviceIds = odapi.get_device_ids()
        device = None
        openDeviceRetry = 0

        #attempt to reprobe and open the device at most 3 times before bailing out.
        while device is None and openDeviceRetry < 3:
            for devId in deviceIds:
                try:
                    device = odapi.open_device(devId)
                    devSerialNumber = device.get_serial_number()

                    if devSerialNumber == serialNumber:
                        break
                    else:
                        device.close_device()
                        device = None
                except OceanDirectError as err:
                    [errorCode, errorMsg] = err.get_error_details()
                    device = None
                    # print("readSpectra(): exception at serial# / error / %s / %d = %s" %
                    #     (serialNumber, errorCode, errorMsg))
            if device is not None:
                break
            else:
                #re-probe for unopened devices again
                time.sleep(1)
                deviceCount = odapi.find_usb_devices()

                if deviceCount == 0:
                    print("ERROR: no device found. Cannot open device with serial number =  %s" % serialNumber)
                    break
                deviceIds = odapi.get_device_ids()
                openDeviceRetry = openDeviceRetry + 1

        if device is not None:
            serialNumber = device.get_serial_number()
            device.set_integration_time(integrationTimeUs)

            print("Started reading spectra with serial#: %s" % serialNumber)
            for i in range(spectraToRead):
                spectra = device.get_formatted_spectrum()
                output.append((serialNumber, spectra))
                print("serial#/spectra:  %s = %d, %d, %d, %d \n" 
                      %(serialNumber, spectra[100], spectra[101], spectra[102], spectra[103]), flush=True)

            print("Closing device with serial#: %s" % serialNumber)
            device.close_device()
    odapi.shutdown()
    return output


def main():
    serialNumberList = read_all_serial_numbers()

    print("Device Serial Numbers:  %s" % ', '.join(map(str, serialNumberList)) )

    if len(serialNumberList) == 0:
        print("No device found.")
    else:
        processList = []
        queueList = []
        with Manager() as mgr:
            for serialNumber in serialNumberList:
                output = mgr.list()
                queueList.append(output)
                processList.append(Process(target=readSpectra, args=(serialNumber, 100000, 20, output)))

            #run all process and read spectra
            for processValue in processList:
                processValue.start()

            #wait for all process to complete
            for processValue in processList:
                processValue.join()

            #print all spectra
            for queueValue in queueList:
                print("Queue size =  %d" % len(queueValue), flush=True )
                for (serialNumber, spectra) in queueValue:
                    print("%s ==> %d, %d, %d" % (serialNumber, spectra[100], spectra[101], spectra[102]), flush=True)

    print("****** end of program ******")

if __name__ == '__main__':
    main()
