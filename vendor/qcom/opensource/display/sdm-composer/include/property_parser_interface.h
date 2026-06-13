/*
Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef __PROPERTY_PARSER_INTERFACE__
#define __PROPERTY_PARSER_INTERFACE__

class PropertyParserInterface {
 public:
  /*! @brief Method to create property parser interface.

    @details This function to be called once per the device life cycle. This function creates
             property parser interface which parses the property and its value from
             vendor_build.prop property file. Property and value should be stored in following
             format "property_name=value" to parse the file appropriately.
    @param[out] intf - Populates property parser interface pointer.

    @return Returns 0 on sucess otherwise errno
  */
  static int Create(PropertyParserInterface **intf);

  /*! @brief Method to destroy property parser interface.

    @details This function to be called once per the device life cycle.

    @param[in] intf - Property parser interface pointer which was populated by Create() function.

    @return Returns 0 on sucess otherwise errno
  */
  static int Destroy(PropertyParserInterface *intf);

  /*! @brief Method to get the value of the property as integer.

    @param[in] property_name - property name for which you want to retrieve the value.
    @param[out] value - Integer pointer stores the value of the property.

    @return Returns 0 on sucess otherwise errno
  */
  virtual int GetProperty(const char *property_name, int *value) = 0;

  /*! @brief Method to get the value of the property as char string.

    @param[in] property_name - property name for which you want to retrieve the value.
    @param[out] value - char pointer stores the value of the property.

    @return Returns 0 on sucess otherwise errno
  */
  virtual int GetProperty(const char *property_name, char *value) = 0;


 protected:
  virtual ~PropertyParserInterface() { }
};

#endif  // __PROPERTY_PARSER_INTERFACE__
