# India Regulatory Documents — LoRa Frequency

## Documents in This Folder

| File | Description |
|------|-------------|
| `WPC_865MHz_License_Free_Notification.pdf` | WPC notification declaring 865–867 MHz as license-free for low-power devices |
| `WPC_SRD_865MHz_License_Exemption_Rules_2021.pdf` | Rules for Short Range Devices (SRD) exemption from licence in 865–868 MHz band (2021) |
| `WPC_Import_Licence_Orders_Compendium_2022.pdf` | Compendium of WPC orders related to import licences for wireless equipment |
| `NFAP_2025_National_Frequency_Allocation_Plan.pdf` | National Frequency Allocation Plan 2025 — complete spectrum allocation reference |

---

## Project Disclaimer

**This project is a college-level academic prototype built for educational purposes only. It is NOT intended for commercial use, public deployment, or any application beyond a controlled lab/campus environment.**

### Why 433 MHz Instead of 865–867 MHz

The legally preferred unlicensed LoRa band in India is **865–867 MHz** as defined by WPC under the Indian Wireless Telegraphy Act. However, this project uses the **SX1278 module operating at 433 MHz** for the following reasons:

1. **Budget constraints** — The SX1278 (433 MHz) is significantly cheaper (₹150–250) compared to SX1276-based 868/915 MHz modules (₹500–900+), making it the only feasible option for a self-funded college project.
2. **Academic scope** — The goal of this project is to demonstrate LoRa-based mesh communication concepts, not spectrum-compliant deployment. The RF behaviour, modulation, and protocol are identical regardless of frequency band.
3. **Prototype only** — This device will never be sold, distributed, or used outside the campus lab environment.

### NFAP Note for 433 MHz

The **National Frequency Allocation Plan (NFAP 2025)** includes references to the **433.05–434.79 MHz** segment as a shared band under Region-3 allocations. For this reason, this project treats 433 MHz usage as a **low-power educational prototype case**, not as a blanket commercial authorization.

Please refer directly to the relevant table/row in `NFAP_2025_National_Frequency_Allocation_Plan.pdf` when preparing formal submissions.

### Intended Use

- Classroom / lab demonstration of LoRa communication
- College final year project / mini-project submission
- Learning embedded systems, RF protocols, and ESP32 firmware development

### Important Notice

> The author acknowledges that 433 MHz LoRa operation in India is in a shared amateur/ISM band and is not explicitly covered under the WPC SRD exemption notification for 865–867 MHz. This hardware will not be operated in any manner that causes interference to licensed services. No commercial service, public network, or paid use of any kind is associated with this project.
>
> If you intend to build a production or commercial system based on this project, **replace the SX1278 with an 865 MHz module** (such as the Ra-02 at 868 MHz or LLCC68-based modules) to comply with WPC regulations.

### References

- WPC Office, Department of Telecommunications, Government of India: `wpc.dot.gov.in`
- Indian Wireless Telegraphy Act, 1933
- WPC SRD Exemption Notification, 2021 (see `WPC_SRD_865MHz_License_Exemption_Rules_2021.pdf`)
- NFAP 2025 (see `NFAP_2025_National_Frequency_Allocation_Plan.pdf`, 433.05–434.79 MHz entries)
