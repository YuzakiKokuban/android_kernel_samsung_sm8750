# Copyright (c) 2017-2022, The Linux Foundation. All rights reserved.
# Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import re
from collections import OrderedDict

from parser_util import register_parser, RamParser
from print_out import print_out_str
from tempfile import NamedTemporaryFile
from .ftrace_event_list import FtraceParser_Event_List
from .ftrace_event import FtraceParser_Event, BufferedWrite
import linux_list as llist
#import time

@register_parser('--dump-ftrace', 'extract ftrace by iterate the ring buffer page',optional=True)
class FtraceParser(RamParser):

    def __init__(self, *args):
        super(FtraceParser, self).__init__(*args)
        self.format_event_map = OrderedDict()
        self.format_event_field_map = OrderedDict()
        self.event_call = 'struct trace_event_call'
        self.event_class = 'struct trace_event_class'
        self.trace_names = ["binder", "bootreceiver", "clock_reg", "kgsl-fence",
                            "memory", "mmc", "rproc_qcom", "suspend", "ufs",
                            "usb", "wifi", "rwmmio"]
        self.whitelisted_trace_names =[]
        self.ftrace_buffer_size_kb = None
        self.per_cpu_buffer_pages = None
        self.savedcmd = self.ramdump.read_pdatatype('savedcmd')

        if len(self.ramdump.ftrace_args):
            self.whitelisted_trace_names = self.ramdump.ftrace_args

        if self.ramdump.ftrace_max_size:
            self.per_cpu_buffer_pages = self.ramdump.ftrace_max_size / 4

    def ftrace_field_func(self, common_list, ram_dump):
        name_offset = ram_dump.field_offset('struct ftrace_event_field', 'name')
        type_offset = ram_dump.field_offset('struct ftrace_event_field', 'type')
        filter_type_offset = ram_dump.field_offset('struct ftrace_event_field', 'filter_type')
        field_offset = ram_dump.field_offset('struct ftrace_event_field', 'offset')
        size_offset = ram_dump.field_offset('struct ftrace_event_field', 'size')
        signed_offset = ram_dump.field_offset('struct ftrace_event_field', 'is_signed')

        name = ram_dump.read_word(common_list + name_offset)
        field_name = ram_dump.read_cstring(name, 256)
        type_name = ram_dump.read_word(common_list + type_offset)
        type_str = ram_dump.read_cstring(type_name, 256)
        offset = ram_dump.read_u32(common_list + field_offset)
        size = ram_dump.read_u32(common_list + size_offset)
        signed = ram_dump.read_u32(common_list + signed_offset)

        if re.match('(.*)\[(.*)', type_str) and not (re.match('__data_loc', type_str)):
            s = re.split('\[', type_str)
            s[1] = '[' + s[1]
            self.formats_out.write(
                "\tfield:{0} {1}{2};\toffset:{3};\tsize:{4};\tsigned:{5};\n".format(s[0], field_name, s[1], offset,
                                                                                    size, signed))
            if "common_type" == field_name or "common_flags" == field_name or "common_preempt_count" == field_name or "common_pid" == field_name:
                temp = 0
            else:
                format_list = []
                format_list.append(type_str)
                format_list.append(offset)
                format_list.append(size)
                self.format_event_field_map[field_name] = format_list
        else:
            self.formats_out.write(
                "\tfield:{0} {1};\toffset:{2};\tsize:{3};\tsigned:{4};\n".format(type_str, field_name, offset, size,

                                                                                 signed))
            #self.format_event_field_map = {}

            if "common_type" == field_name or "common_flags" == field_name or "common_preempt_count" == field_name or "common_pid" == field_name:
                temp = 0
            else:
                format_list = []
                format_list.append(type_str)
                format_list.append(offset)
                format_list.append(size)
                self.format_event_field_map[field_name] = format_list

    def ftrace_events_func(self, ftrace_list, ram_dump):
        event_offset = ram_dump.field_offset(self.event_call, 'event')
        fmt_offset = ram_dump.field_offset(self.event_call, 'print_fmt')
        class_offset = ram_dump.field_offset(self.event_call, 'class')
        flags_offset = ram_dump.field_offset(self.event_call, 'flags')
        flags = ram_dump.read_word(ftrace_list + flags_offset)
        if ram_dump.kernel_version >= (4, 14):
            TRACE_EVENT_FL_TRACEPOINT = 0x10
        elif ram_dump.kernel_version >= (4, 9):
            TRACE_EVENT_FL_TRACEPOINT = 0x20
        else:
            TRACE_EVENT_FL_TRACEPOINT = 0x40
        if (ram_dump.kernel_version >= (3, 18) and (flags & TRACE_EVENT_FL_TRACEPOINT)):
            tp_offset = ram_dump.field_offset(self.event_call, 'tp')
            tp_name_offset = ram_dump.field_offset('struct tracepoint', 'name')
            tp = ram_dump.read_word(ftrace_list + tp_offset)
            name = ram_dump.read_word(tp + tp_name_offset)
        else:
            name_offset = ram_dump.field_offset(self.event_call, 'name')
            name = ram_dump.read_word(ftrace_list + name_offset)

        type_offset = ram_dump.field_offset('struct trace_event', 'type')
        fields_offset = ram_dump.field_offset(self.event_class, 'fields')
        common_field_list = ram_dump.address_of('ftrace_common_fields')
        field_next_offset = ram_dump.field_offset('struct ftrace_event_field', 'link')

        name_str = ram_dump.read_cstring(name, 512)
        event_id = ram_dump.read_word(ftrace_list + event_offset + type_offset)
        fmt = ram_dump.read_word(ftrace_list + fmt_offset)
        fmt_str = ram_dump.read_cstring(fmt, 2048)

        self.formats_out.write("name: {0}\n".format(name_str))
        self.formats_out.write("ID: {0}\n".format(event_id))
        self.formats_out.write("format:\n")

        #self.format_event_map[name_str] = format_event_field_map

        list_walker = llist.ListWalker(ram_dump, common_field_list, field_next_offset)
        list_walker.walk_prev(common_field_list, self.ftrace_field_func, ram_dump)
        self.formats_out.write("\n")

        event_class = ram_dump.read_word(ftrace_list + class_offset)
        field_list = event_class + fields_offset
        list_walker = llist.ListWalker(ram_dump, field_list, field_next_offset)
        list_walker.walk_prev(field_list, self.ftrace_field_func, ram_dump)
        self.formats_out.write("\n")
        self.formats_out.write("print fmt: {0}\n".format(fmt_str))
        fmt_list = []
        fmt_list.append(self.format_event_field_map)
        fmt_list.append(fmt_str)
        self.format_event_map[name_str] = fmt_list
        self.format_event_field_map = OrderedDict()

    def ftrace_get_format(self):
        self.formats_out = self.ramdump.open_file('formats.txt')
        fevent_list = FtraceParser_Event_List(self.ramdump)
        #print(fevent_list.ftrace_raw_struct_type)

        ftrace_events_list = self.ramdump.address_of('ftrace_events')
        next_offset = self.ramdump.field_offset(self.event_call, 'list')
        list_walker = llist.ListWalker(self.ramdump, ftrace_events_list, next_offset)
        list_walker.walk_prev(ftrace_events_list, self.ftrace_events_func, self.ramdump)
        self.formats_out.close()
        return fevent_list

    def ftrace_extract(self):
        #ftrace_event_time = 0
        #post_ftrace_event_time = 0
        #taskdump_time = 0
        #parse_trace_entry_time = 0
        global_trace_data_org = self.ramdump.address_of('ftrace_trace_arrays')
        global_trace_data_offset = self.ramdump.field_offset(
            'struct list_head ', 'next')
        global_trace_data_next = self.ramdump.read_pointer(global_trace_data_org + global_trace_data_offset)
        if self.ramdump.kernel_version >= (5, 10):
            trace_buffer_offset = self.ramdump.field_offset(
                'struct trace_array', 'array_buffer')
        else:
            trace_buffer_offset = self.ramdump.field_offset(
                'struct trace_array', 'trace_buffer')
        trace_buffer_name_offset = self.ramdump.field_offset(
            'struct trace_array', 'name')
        if self.ramdump.kernel_version >= (5, 10):
            ring_trace_buffer_ptr = self.ramdump.field_offset(
                'struct array_buffer', 'buffer')
        else:
            ring_trace_buffer_ptr = self.ramdump.field_offset(
                'struct trace_buffer', 'buffer')
        if self.ramdump.kernel_version >= (5, 10):
            ring_trace_buffer_cpus_ptr = self.ramdump.field_offset(
                'struct trace_buffer', 'cpus')
            ring_trace_buffer_base_addr = self.ramdump.field_offset(
                'struct trace_buffer', 'buffers')
        else:
            ring_trace_buffer_cpus_ptr = self.ramdump.frame_field_offset(
                'rb_wake_up_waiters','struct ring_buffer', 'cpus')
            if ring_trace_buffer_cpus_ptr is None:
                ring_trace_buffer_cpus_ptr = 0x4
            ring_trace_buffer_base_addr = self.ramdump.frame_field_offset(
                'rb_wake_up_waiters','struct ring_buffer', 'buffers')
            if ring_trace_buffer_base_addr is None:
                ring_trace_buffer_base_addr = self.ramdump.field_offset(
                        'struct ring_buffer', 'buffers')
            if ring_trace_buffer_base_addr is None:
                if self.ramdump.arm64:
                    ring_trace_buffer_base_addr = 0x58
                else:
                    ring_trace_buffer_base_addr = 0x38
        ring_trace_buffer_nr_pages = self.ramdump.field_offset(
            'struct ring_buffer_per_cpu', 'nr_pages')
        log_pattern = re.compile(r'\s*(.*)-(\d+)\s*\[(\d+)\]\s*.*')
        fevent_list = self.ftrace_get_format();
        while(global_trace_data_org != global_trace_data_next):
            trace_array = global_trace_data_next
            #print("v.v (struct trace_array)0x%x" %(trace_array))
            trace_buffer_name = self.ramdump.read_word(trace_array + trace_buffer_name_offset)
            if not (trace_buffer_name):
                trace_name = None
            else:
                trace_name = self.ramdump.read_cstring(trace_buffer_name, 256)

            trace_buffer_ptr_data = self.ramdump.read_pointer(trace_array + trace_buffer_offset)
            ring_trace_buffer_data = trace_buffer_ptr_data + trace_buffer_offset
            ring_trace_buffer_base_data = self.ramdump.read_pointer(ring_trace_buffer_data + ring_trace_buffer_ptr)
            ring_trace_buffer_base_data1 = self.ramdump.read_pointer(ring_trace_buffer_base_data + ring_trace_buffer_base_addr)


            if trace_name is None or trace_name == 0x0 or trace_name == "0x0" or trace_name == "None" or trace_name == "null" or len(trace_name) < 1:
                #ftrace_out = self.ramdump.open_file('ftrace.txt','w')
                fout = self.ramdump.open_file('ftrace.txt','w')
                ftrace_out = BufferedWrite(fout)
                header_data = "# tracer: nop \n" \
                              "#\n" \
                              "# entries-in-buffer/entries-written: 315882/1727030   #P:8\n" \
                              "#\n" \
                              "#                              _-----=> irqs-off\n" \
                              "#                             / _----=> need-resched\n" \
                              "#                            | / _---=> hardirq/softirq\n" \
                              "#                            || / _--=> preempt-depth\n" \
                              "#                            ||| /     delay\n" \
                              "#           TASK-PID   CPU#  ||||    TIMESTAMP  FUNCTION\n" \
                              "#              | |       |   ||||       |         |\n"
                ftrace_out.write(header_data)
            else:
                if trace_name in self.whitelisted_trace_names or self.whitelisted_trace_names == ["all"]:
                    #ftrace_out = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '.txt','w')
                    fout = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '.txt','w')
                    ftrace_out = BufferedWrite(fout)
                else:
                    global_trace_data_next =  self.ramdump.read_pointer(global_trace_data_next)
                    continue
            #    ftrace_out = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '.txt','w')

            ftrace_time_data = {}
            nr_total_buffer_pages = 0
            rb_per_cpu = []
            nr_pages_per_buffer = []
            #taskdump.do_dump_stacks(self.ramdump, 0)
            for cpu_idx in range(0,8):
                #array_ptr = self.ramdump.read_u64(ring_trace_buffer_base_data1 + self.ramdump.sizeof('void *') * cpu_idx)
                array_ptr = (ring_trace_buffer_base_data1 + self.ramdump.sizeof('void *') * cpu_idx)
                b = self.ramdump.read_pointer(array_ptr)
                if b is None or b == 0x0:
                    continue
                if self.ramdump.arm64:
                    nr_pages =  self.ramdump.read_u64(
                        b + ring_trace_buffer_nr_pages)
                else:
                    nr_pages =  self.ramdump.read_u32(
                        b + ring_trace_buffer_nr_pages)
                if nr_pages is None:
                    continue
                if self.per_cpu_buffer_pages and self.per_cpu_buffer_pages < nr_pages:
                    nr_pages = self.per_cpu_buffer_pages

                nr_total_buffer_pages = nr_total_buffer_pages +  nr_pages

                nr_pages_per_buffer.append(nr_pages)
                rb_per_cpu.append(b)
                #print "ring_trace_buffer_cpus nr_pages = %d" % nr_pages
                #print "cpu_buffer = {0}".format(hex(b))

            print("\nTotal pages across cpu trace buffers = {}".format(round(nr_total_buffer_pages)))

            #start = time.time()
            for cpu_idx in range(0,len(rb_per_cpu)):
                nr_pages_per_buffer_item = nr_pages_per_buffer[cpu_idx]
                per_cpu_buffer = rb_per_cpu[cpu_idx]
                if per_cpu_buffer is not None:
                    evt = FtraceParser_Event(self.ramdump,ftrace_out,cpu_idx,fevent_list.ftrace_event_type,fevent_list.ftrace_raw_struct_type,ftrace_time_data,self.format_event_map,self.savedcmd)
                    evt.ring_buffer_per_cpu_parsing(per_cpu_buffer)
                    #parse_trace_entry_time += evt.parse_trace_entry_time
            #ftrace_event_time += (time.time()-start)
            global_trace_data_next =  self.ramdump.read_pointer(global_trace_data_next)
            switch_map = {}
            ftrace_file_map = {}
            if trace_name is None or trace_name == 0x0 or trace_name == "0x0" or trace_name == "None" or trace_name == "null" or len(trace_name) < 1:
                ftrace_core0_fd = self.ramdump.open_file('ftrace_core0.txt', 'w')
                ftrace_core1_fd = self.ramdump.open_file('ftrace_core1.txt', 'w')
                ftrace_core2_fd = self.ramdump.open_file('ftrace_core2.txt', 'w')
                ftrace_core3_fd = self.ramdump.open_file('ftrace_core3.txt', 'w')
                ftrace_core4_fd = self.ramdump.open_file('ftrace_core4.txt', 'w')
                ftrace_core5_fd = self.ramdump.open_file('ftrace_core5.txt', 'w')
                ftrace_core6_fd = self.ramdump.open_file('ftrace_core6.txt', 'w')
                ftrace_core7_fd = self.ramdump.open_file('ftrace_core7.txt', 'w')
            else:
                if trace_name in self.whitelisted_trace_names or self.whitelisted_trace_names == ["all"]:
                    ftrace_core0_fd = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '_core0.txt','w')
                    ftrace_core1_fd = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '_core1.txt','w')
                    ftrace_core2_fd = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '_core2.txt','w')
                    ftrace_core3_fd = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '_core3.txt','w')
                    ftrace_core4_fd = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '_core4.txt','w')
                    ftrace_core5_fd = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '_core5.txt','w')
                    ftrace_core6_fd = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '_core6.txt','w')
                    ftrace_core7_fd = self.ramdump.open_file('ftrace_parser/' + 'ftrace_' + trace_name + '_core7.txt','w')
                else:
                    continue

            ftrace_file_map["000"] = BufferedWrite(ftrace_core0_fd)
            ftrace_file_map["001"] = BufferedWrite(ftrace_core1_fd)
            ftrace_file_map["002"] = BufferedWrite(ftrace_core2_fd)
            ftrace_file_map["003"] = BufferedWrite(ftrace_core3_fd)
            ftrace_file_map["004"] = BufferedWrite(ftrace_core4_fd)
            ftrace_file_map["005"] = BufferedWrite(ftrace_core5_fd)
            ftrace_file_map["006"] = BufferedWrite(ftrace_core6_fd)
            ftrace_file_map["007"] = BufferedWrite(ftrace_core7_fd)

            #start = time.time()
            sorted_dict = {k: ftrace_time_data[k] for k in sorted(ftrace_time_data)}
            for key in sorted(sorted_dict.keys()):
                for i in range(0,len(ftrace_time_data[key])):
                    line = str(ftrace_time_data[key][i])
                    replaced_line = line
                    trace_log = log_pattern.match(line)
                    bestguess_pid = None
                    bestguess_comm = None
                    if bool(trace_log):
                        cpu_number = trace_log.group(3)
                        entry_pid = trace_log.group(2)
                    else:
                        cpu_number = None
                        entry_pid = None

                    if "sched_switch:" in line:
                        prev_comm = line.split("prev_comm=")[1].split(" ")[0]
                        prev_pid = line.split("prev_pid=")[1].split(" ")[0]
                        curr_comm = line.split("next_comm=")[1].split(" ")[0]
                        curr_pid = line.split("next_pid=")[1].split(" ")[0]
                        if cpu_number not in switch_map:
                            switch_map[cpu_number] = {}
                        switch_map[cpu_number]["comm"] = curr_comm
                        switch_map[cpu_number]["pid"] = curr_pid
                        bestguess_pid = prev_pid
                        bestguess_comm = prev_comm
                    elif "<TBD>" in line and cpu_number in switch_map:
                        bestguess_comm = switch_map[cpu_number]["comm"]
                        bestguess_pid = switch_map[cpu_number]["pid"]

                    if "<TBD>" in line:
                        if entry_pid is not None and bestguess_pid is not None and int(entry_pid) == int(bestguess_pid):
                            replaced_line = line.replace("<TBD>", bestguess_comm)
                        else:
                            replaced_line = line.replace("<TBD>", "<...>")
                    ftrace_out.write(replaced_line)
                    ftrace_file_map[str(cpu_number)].write(replaced_line)
            #post_ftrace_event_time += (time.time()-start)
        #print("Ftrace Event Parsing took {} secs".format(ftrace_event_time))
        #print("Post Ftrace Event Sorting and Write took {} secs".format(post_ftrace_event_time))
        #print("Parse Ftrace Entry function took {} secs".format(parse_trace_entry_time))


    def parse(self):
        if self.ramdump.ftrace_limit_time == 0:
            self.ftrace_extract()
        else:
            from func_timeout import func_timeout
            print_out_str("Limit ftrace parser running time to {}s".format(self.ramdump.ftrace_limit_time))
            func_timeout(self.ramdump.ftrace_limit_time, self.ftrace_extract)
