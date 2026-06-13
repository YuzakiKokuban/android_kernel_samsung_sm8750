/*
 *Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/


package vendor.qti.hardware.qspa;
import vendor.qti.hardware.qspa.PartInfo;
@VintfStability
interface IQspa {

PartInfo[] get_all_subparts();
String[] get_available_parts();
int[] get_subpart_info(in String part);
int get_num_available_cpus();
int get_num_available_clusters();
int get_num_physical_clusters();
int[] get_cpus_of_physical_clusters(in int physical_cluster_number);

}