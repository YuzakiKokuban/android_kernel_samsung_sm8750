/*==============================================================================
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*===============================================================================
*/

#include <regex>
#include <string>
#include <fstream>
#include <errno.h>
#include <unistd.h>
#include <utils/Log.h>
#include <cutils/uevent.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "UsbUdevService"

#define UEVENT_MSG_LEN  2048

const int MAX_UEVENT_BUFFER_LENGTH = 64*1024;
static int uevent_fd = -1;

const std::string PERIPHERAL_PRODUCT_ID= "9131";
const std::string PERIPHERAL_VENDOR_ID_UEVENT = "5c6";
// There is difference in Product id sent in UEVENT and stored at USB_GADGET_PRODUCT_ID_PATH
const std::string PERIPHERAL_VENDOR_ID_PROC = "05c6";
const std::string HOST_IP_ADDRESS = "192.168.1.10";
const std::string USB_GADGET_VENDOR_ID_PATH = "/sys/bus/usb/devices/usb2/2-1/idVendor";
const std::string USB_GADGET_PRODUCT_ID_PATH = "/sys/bus/usb/devices/usb2/2-1/idProduct";

class USB_uevent{

  private:
    std::string device_id;
    std::string interface;
    std::string driver;
    std::string devpath;
    std::string action;

  public:
    void update_value(const std::string &key,const std::string &value){
      if (key == "PRODUCT")
          this->device_id = value.c_str();
      else if ( key =="INTERFACE")
          this->interface = value.c_str();
      else if ( key =="DRIVER")
          this->driver = value.c_str();
      else if ( key =="DEVPATH")
          this->devpath = value.c_str();
      else if ( key =="ACTION")
          this->action = value.c_str();
    }

    std::string get_device_id(){
      return this->device_id;
    }
    std::string get_interface(){
      return this->interface;
    }
    std::string get_driver(){
      return this->driver;
    }
    std::string get_devpath(){
      return this->devpath;
    }
    std::string get_action(){
      return this->action;
    }
};

std::string get_file_contents(const std::string &file_path){
  std::fstream file_handler;
  file_handler.open(file_path,std::fstream::in);
  std::string data;
  if ( file_handler.is_open() ) {
      file_handler >> data;
      return data;
  }else{
    ALOGE("Unable to read file at %s\n",file_path.c_str());
    ALOGE("Error is : %s\n",strerror(errno));
  }
  return "";
}

static void parse_uevent( char *msg, int n,USB_uevent &uevent){
    if (msg == NULL)
      return;
    std::string delimiter = "=";
    char *p = msg;
    p += strlen(p)+1;
    for (int i=0;*p!='\0' && i<n; i++) {
        std::string line = std::string(p);
        size_t pos = 0;
        pos = line.find(delimiter);
        if (pos == std::string::npos){
           ALOGE("Invalid Uevent data : %s\n",line.c_str());
        }
        std::string key = line.substr(0, pos);
        line.erase(0,pos+delimiter.length());
        std::string value = line;
        uevent.update_value(key,value);
        p+=key.length()+value.length()+delimiter.length()+1;
    }
}

static bool is_product_id(const std::string &device_product_id,const std::string &aurora_product_id){
  return device_product_id==aurora_product_id;
}

static bool is_vendor_id(const std::string &device_vendor_id,const std::string &aurora_vendor_id){
  return device_vendor_id==aurora_vendor_id;
}

static bool is_device_aurora(const std::string &sdevice_id){
  std::string delimiter = "/";
  std::string device_id = sdevice_id;
  size_t pos = 0;
  std::string substring;
  pos = device_id.find(delimiter);
  if (  pos == std::string::npos){
    return false;
  }
  std::string device_vendor_id = device_id.substr(0, pos);
  if (!is_vendor_id(device_vendor_id,PERIPHERAL_VENDOR_ID_UEVENT)){
    return false;
  }
  device_id.erase(0,pos+delimiter.length());
   pos = device_id.find(delimiter);
  if (  pos == std::string::npos){
    return false;
  }
  std::string device_product_id = device_id.substr(0, pos);
  return is_product_id(device_product_id,PERIPHERAL_PRODUCT_ID);
}

static void trim_string_back(std::string &str){
  while (str.back() == '\n' ||
         str.back() == ' ' ||
         str.back() == '\t' ||
         str.back() == '\r')
         str.pop_back();
}

static void set_device_ip(){
  sleep(1);
  std::string set_ip_command = "/system/bin/ifconfig usb0 up "+HOST_IP_ADDRESS;
  int rval = std::system(set_ip_command.c_str());
  uint8_t SET_IP_RETRY_COUNT = 3;
  while(rval && SET_IP_RETRY_COUNT>0){
    ALOGE("Failed to update IP Adress");
    ALOGE("return code is : %d\n",rval);
    ALOGE("Error is : %s\n",strerror(errno));
    SET_IP_RETRY_COUNT--;
    sleep(1);
    rval = std::system(set_ip_command.c_str());
  }
  if(rval==0) ALOGE("Successfully assigned IP Address");
}

static void update_routing_table(){
  sleep(1);
  std::string route_cmd = "/system/bin/ip rule add from all lookup main prio 9999";
  int rval = std::system(route_cmd.c_str());
  uint8_t UPDATE_IP_RULE_RETRY_COUNT = 3;
  while(rval && UPDATE_IP_RULE_RETRY_COUNT>0){
    ALOGE("Failed to update routing-table");
    ALOGE("return code is : %d\n",rval);
    ALOGE("Error is : %s\n",strerror(errno));
    UPDATE_IP_RULE_RETRY_COUNT--;
    sleep(1);
    rval = std::system(route_cmd.c_str());
  }
  if(rval==0) ALOGE("Successfully updated routing Table");
}

bool is_peripheral_connected(){
  std::string usb_gadget_vendor_id = get_file_contents(USB_GADGET_VENDOR_ID_PATH);
  trim_string_back(usb_gadget_vendor_id);
  if(usb_gadget_vendor_id!=PERIPHERAL_VENDOR_ID_PROC) return false;
  std::string usb_gadget_product_id = get_file_contents(USB_GADGET_PRODUCT_ID_PATH);
  trim_string_back(usb_gadget_product_id);
  return usb_gadget_product_id == PERIPHERAL_PRODUCT_ID;
}

static void handle_usb_uevent(char *msg, int n){
  std::regex bind_xhci_hcd_regex("bind@(/devices/platform/soc/.*dwc3/xhci-hcd\\.\\d\\.auto/"
                              "usb\\d/\\d-\\d(?:/[\\d\\.-]+)*)");
  std::cmatch match;
  if (std::regex_match(msg, match, bind_xhci_hcd_regex)){
    USB_uevent uevent;
    parse_uevent(msg,n,uevent);
    std::string device_id = uevent.get_device_id();
    if (is_device_aurora(device_id)){
      set_device_ip();
      update_routing_table();
    }
  }
}

static void start_uevent_client() {
    int buff_length;
    char msg[UEVENT_MSG_LEN + 2];
    uevent_fd = uevent_open_socket(MAX_UEVENT_BUFFER_LENGTH, true);
    if (uevent_fd < 0) {
      ALOGE("uevent_open_socket failed\n");
      return;
    }
    if(is_peripheral_connected()){
        set_device_ip();
        update_routing_table();
    }
    while ((buff_length = uevent_kernel_multicast_recv(uevent_fd, msg, UEVENT_MSG_LEN)) > 0) {
      if (buff_length >= UEVENT_MSG_LEN) { // overflow -- discard
        continue;
      }
      msg[buff_length] = '\0';
      msg[buff_length+1] = '\0';
      handle_usb_uevent(msg,buff_length);
    }
}

int main(){
        start_uevent_client();
        return 0;
}
