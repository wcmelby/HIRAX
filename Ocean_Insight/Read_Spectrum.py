
import time, os, csv
import numpy as np
from multiprocessing import Process, Manager
from oceandirect.OceanDirectAPI import OceanDirectAPI, OceanDirectError, Spectrometer
odapi = OceanDirectAPI()

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


def correct_nonlinearity(raw_intensity, nonlinearity_coeffs):
    """Corrects for nonlinearity using the coefficients provided by the spectrometer."""
    raw_intensity = np.array(raw_intensity, dtype=float)
    corrected_intensity = raw_intensity.copy()
    # corrected_intensity = np.array(corrected_intensity)

    for i, coeff in enumerate(nonlinearity_coeffs):
        corrected_intensity += coeff * raw_intensity**(i + 1)
        
    return corrected_intensity


def correct_spectrum(raw_spectrum):
    """Corrects the spectrum for nonlinearity and converts pixel values to wavelengths."""
    deviceCount = odapi.find_usb_devices()
    if deviceCount == 0:
        raise Exception("No devices found")
    elif deviceCount > 1:
        raise Exception("More than one device found")
    elif deviceCount == 1:
        device_id = odapi.get_device_ids()
        if not device_id:
            raise Exception("No devices found")

    device = odapi.open_device(device_id[0])  # Initialize your device object here
    spectrometer_advanced = Spectrometer.Advanced(device)

    wavelength_coeffs = spectrometer_advanced.get_wavelength_coeffs()
    nonlinearity_coeffs = spectrometer_advanced.get_nonlinearity_coeffs()
    c = wavelength_coeffs
    num_data_points = len(raw_spectrum) # 3648

    pixels = np.arange(num_data_points)
    wavelengths = c[0] + c[1] * pixels + c[2] * pixels**2 + c[3] * pixels**3 # polynomial equation to convert pixel values to wavelengths

    correct_spectrum = correct_nonlinearity(raw_spectrum, nonlinearity_coeffs)

    odapi.close_device(device_id[0])

    return wavelengths, correct_spectrum


def readSpectra(serialNumber: str, integrationTimeUs: int, spectraToRead: int, spectrum_file_name : str) -> None:
    """
    This function will be executed in a separate process. This will open the device that matched the 
    given serial number.
    Returns output (list of spectrum) as well as data in a newly created csv file. 
    This is set up to just read one spectrometer. 
    """
    output = []
    odapi = OceanDirectAPI()
    deviceCount = odapi.find_usb_devices()
    os.chdir('./Spectra_Files')
    spectrum_file = os.path.join(os.getcwd(), spectrum_file_name)

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
            print("Reading spectra with serial#: %s" % serialNumber)

            for i in range(spectraToRead): # take this line out, only one spectru
                spectra = device.get_formatted_spectrum()
                output.append((serialNumber, spectra))
                data = output[0]
                raw_spectrum = data[1:]
                raw_spectrum = raw_spectrum[0]

                # Write the spectra to a csv file
                with open(spectrum_file, 'w', newline='') as csvfile:
                    csv_writer = csv.writer(csvfile)
                    csv_writer.writerow(['Wavelength', 'Intensity'])  # Write the header
                    wavelengths = correct_spectrum(raw_spectrum)[0]
                    spectrum = correct_spectrum(raw_spectrum)[1]
                    if len(wavelengths) == len(spectrum):
                        for wavelength, intensity in zip(wavelengths, spectrum):
                            csv_writer.writerow([wavelength, intensity])
                    else:
                        print("Error: Wavelengths and spectrum arrays are not of the same length.")

            print("Closing device with serial#: %s" % serialNumber)
            device.close_device()

    odapi.shutdown()
    return wavelengths, spectrum


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
