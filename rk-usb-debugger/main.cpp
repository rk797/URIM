#include <iostream>
#include <vector>
#include <iomanip>
#include <hidapi.h>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <windows.h>
#include <sstream>

void PrintWithLineDelay(const std::string& text, int delayNanoseconds = 100) {
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        std::cout << line << std::endl;
        std::this_thread::sleep_for(std::chrono::nanoseconds(delayNanoseconds));
    }
}

// Adjust the function to handle indentation for nested collections.
void PrintReportDescriptor(const std::vector<unsigned char>& reportDescriptor, int indentLevel = 0) {
    size_t i = 0;
    int collectionDepth = 0;

    while (i < reportDescriptor.size()) {
        unsigned char prefix = reportDescriptor[i];
        unsigned char sizeCode = prefix & 0x03;
        unsigned char type = (prefix >> 2) & 0x03;
        unsigned char tag = (prefix >> 4) & 0x0F;

        size_t dataSize;
        switch (sizeCode) {
        case 0:
            dataSize = 0;
            break;
        case 1:
        case 2:
            dataSize = sizeCode;
            break;
        case 3:
            dataSize = 4;
            break;
        default:
            dataSize = 0;
            break;
        }

        std::stringstream output;
        // Indentation based on the current collection depth.
        output << std::string(indentLevel + collectionDepth * 2, ' ');
        output << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(prefix) << " ";

        for (size_t j = 0; j < dataSize; ++j) {
            if (i + 1 + j < reportDescriptor.size()) {
                unsigned char dataByte = reportDescriptor[i + 1 + j];
                output << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(dataByte) << " ";
            }
        }

        std::string label = "Unknown";
        switch (type) {
        case 0:  // Main
            switch (tag) {
            case 8:  label = "Input"; break;
            case 9:  label = "Output"; break;
            case 11: label = "Feature"; break;
            case 10: label = "Collection"; collectionDepth++; break;
            case 12: label = "End Collection"; collectionDepth--; break;
            default: label = "Main"; break;
            }
            break;
        case 1:  // Global
            switch (tag) {
            case 0:  label = "Usage Page"; break;
            case 1:  label = "Logical Minimum"; break;
            case 2:  label = "Logical Maximum"; break;
            case 3:  label = "Physical Minimum"; break;
            case 4:  label = "Physical Maximum"; break;
            case 5:  label = "Unit Exponent"; break;
            case 6:  label = "Unit"; break;
            case 7:  label = "Report Size"; break;
            case 8:  label = "Report ID"; break;
            case 9:  label = "Report Count"; break;
            case 10: label = "Push"; break;
            case 11: label = "Pop"; break;
            default: label = "Global"; break;
            }
            break;
        case 2:  // Local
            switch (tag) {
            case 0:  label = "Usage"; break;
            case 1:  label = "Usage Minimum"; break;
            case 2:  label = "Usage Maximum"; break;
            case 3:  label = "Designator Index"; break;
            case 4:  label = "Designator Minimum"; break;
            case 5:  label = "Designator Maximum"; break;
            case 7:  label = "String Index"; break;
            case 8:  label = "String Minimum"; break;
            case 9:  label = "String Maximum"; break;
            case 10: label = "Delimiter"; break;
            default: label = "Local"; break;
            }
            break;
        case 3:  // Reserved
            label = "Reserved";
            break;
        }

        std::string line = output.str();
        std::string descriptorLabel = "/* " + label + " */";
        line += std::string(45 - line.length(), ' ') + descriptorLabel;
        std::cout << line << std::endl;

        i += 1 + dataSize;
    }
}

// Function to print device information
void PrintDeviceInfo(struct hid_device_info* dev) {
    std::wstringstream ws;
    ws << L"\n[Device Info]\n";
    ws << L"VID: 0x" << std::uppercase << std::hex << dev->vendor_id << std::dec << L"\n";
    ws << L"PID: 0x" << std::uppercase << std::hex << dev->product_id << std::dec << L"\n";
    ws << L"Manufacturer: " << (dev->manufacturer_string ? dev->manufacturer_string : L"(null)") << L"\n";
    ws << L"Product: " << (dev->product_string ? dev->product_string : L"(null)") << L"\n";
    ws << L"Serial: " << (dev->serial_number ? dev->serial_number : L"(null)") << L"\n";
    ws << L"Path: " << dev->path << L"\n";
    ws << L"Interface Number: " << dev->interface_number << L"\n\n";  // Print the interface number
    std::wstring wideText = ws.str();
    std::string text(wideText.begin(), wideText.end());
    PrintWithLineDelay(text);
}

// Main function to enumerate mouse devices and print the report descriptor with proper formatting.
void FetchMouseDevices() {
    if (hid_init() != 0) {
        PrintWithLineDelay("Failed to initialize HIDAPI\n");
        return;
    }

    struct hid_device_info* devs = hid_enumerate(0x0, 0x0);
    struct hid_device_info* cur_dev = devs;

    while (cur_dev) {
        // Check for HID-compliant mouse based on usage_page (0x01 for Generic Desktop Controls)
        // and usage (0x02 for Mouse)
        if (cur_dev->usage_page == 0x01 && cur_dev->usage == 0x02) {
            PrintDeviceInfo(cur_dev);

            hid_device* device = hid_open_path(cur_dev->path);
            if (device) {
                std::vector<unsigned char> reportDescriptor(HID_API_MAX_REPORT_DESCRIPTOR_SIZE);
                int res = hid_get_report_descriptor(device, reportDescriptor.data(), reportDescriptor.size());
                if (res >= 0) {
                    reportDescriptor.resize(res);
                    std::cout << "[Report Descriptor]\n";
                    PrintReportDescriptor(reportDescriptor, 2);  // Indentation of 2 for better readability
                }
                else {
                    PrintWithLineDelay("[-] Failed to get report descriptor\n");
                }
                hid_close(device);
            }
            else {
                PrintWithLineDelay("[-] Failed to open device\n");
            }
        }
        cur_dev = cur_dev->next;
    }

    hid_free_enumeration(devs);
    hid_exit();
}

int main() {
    FetchMouseDevices();
    PrintWithLineDelay("[+] Press any key to exit\n");
    std::cin.get();
    return 0;
}
