#!/bin/perl
# SPDX-License-Identifier: GPL-2.0
# COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.

# NOTE: 'androidboot.load_modules_parallel=false' is required

use strict;

my @__highest = (
    'drivers/samsung/debug/boot_stat/',
    'drivers/samsung/debug/log_buf/',
    'drivers/samsung/debug/common/',
    'drivers/samsung/debug/arm64/ap_context/',
);
my @__lowest = (
    'drivers/samsung/bsp/qcom/param/',
    'drivers/samsung/debug/qcom/dbg_partition/',
);
my %highest = map { $_ => [] } @__highest if $#__highest >= 0;
my %lowest = map { $_ => [] } @__lowest if $#__lowest >= 0;
my @modules;

reorder_main();

sub reorder_main
{
    my $fd;
    my $dot_ko;
    my $collected;

    open($fd, $ARGV[0]);
    while (<$fd>) {
        $dot_ko = $_;
        $dot_ko =~ s/\s*$//;

        $collected = __collect_highest($dot_ko);
        $collected += __collect_lowest($dot_ko);

        if ($collected == 0) {
            push(@modules, $dot_ko);
        }
    }
    close($fd);

    __print_highest();
    __print_modules();
    __print_lowest();
}

sub __collect_highest
{
    my $_dot_ko = $_[0];
    my $path;

    foreach $path (@__highest) {
        if ($_dot_ko =~ /^kernel\/\Q$path\E/) {
            push(@{$highest{$path}}, $_dot_ko);
            return 1;
        }
    }

    return 0;
}

sub __collect_lowest
{
    my $_dot_ko = $_[0];
    my $path;

    foreach $path (@__lowest) {
        if ($_dot_ko =~ /^kernel\/\Q$path\E/) {
            push(@{$lowest{$path}}, $_dot_ko);
            return 1;
        }
    }

    return 0;
}

sub __print_highest
{
    my ($path, $dot_ko);

    foreach $path (@__highest) {
        foreach $dot_ko (@{$highest{$path}}) {
            printf("%s\n", $dot_ko);
        }
    }
}

sub __print_modules
{
    my $dot_ko;

    foreach $dot_ko (@modules) {
        printf("%s\n", $dot_ko);
    }
}

sub __print_lowest
{
    my ($path, $dot_ko);

    foreach $path (@__lowest) {
        foreach $dot_ko (@{$lowest{$path}}) {
            printf("%s\n", $dot_ko);
        }
    }
}
