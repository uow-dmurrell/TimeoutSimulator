#!/usr/bin/perl -w 
use Data::Dumper;

my $var1 = {
          'general' => {
                         'total_flows' => 3439265,
                         'memory_time' => '1745611899.20461'
                       },
          'mbet' => {
                      'time_waste' => 14887468,
                      'shortflow' => 208653,
                      'shortened' => 766627,
                      'memory_time' => '394685191.239844'
                    },
          'claffy' => {
                        'time_waste' => 8830528,
                        'shortflow' => 137977,
                        'shortened' => 569468,
                        'memory_time' => '268054571.227276'
                      },
          'netramet' => {
                          'time_waste' => '1002973.374095',
                          'shortflow' => 358978,
                          'shortened' => 1705511,
                          'memory_time' => '23536433.5450662'
                        },
          'netramet_short' => {
                                'time_waste' => '814983.355696003',
                                'shortflow' => 644296,
                                'shortened' => 2919967,
                                'memory_time' => '10817634.9743632'
                              },
          'dots' => {
                      'time_waste' => 8826240,
                      'shortflow' => 137986,
                      'shortened' => 569755,
                      'memory_time' => '267984735.145442'
                    },
          'rfc' => {
                     'time_waste' => 140348520,
                     'shortflow' => 148455,
                     'shortened' => 557544,
                     'memory_time' => '5197622540.95389'
                   },
          'pgat' => {
                      'time_waste' => '10904862.112589',
                      'shortflow' => 204622,
                      'shortened' => 765374,
                      'memory_time' => '379598579.521686'
                    },
          'claffy_tcp' => {
                            'time_waste' => 7156672,
                            'shortflow' => 175319,
                            'shortened' => 677195,
                            'memory_time' => '254599626.034063'
                          }
        };

        my @name;
        my @value;
        my $skipa = 0;
        my $skipb = 0;
        foreach my $x (keys(%{$var1})){
            
            if($skipa == 0){  
                push @name,"name";
            }
            
            if($skipb == 0 && $skipa != 0){
                push @name,"name";
               
            }
            push @value,$x;
            
            foreach my $y (keys(%{$var1->{$x}})){
                    if($skipa == 0){ push @name,$y; }
                    if($skipb == 0 && $skipa != 0){ push @name,$y;  }
                    
                    push @value,$var1->{$x}{$y};

                
                   # print $y . "," . $var1->{$x}{$y}."\n";
            }
            
            if($skipb == 0 && $skipa != 0){ $skipb = 1; }
            if(scalar(@name) > 0){
                foreach my $u (@name){
                    print $u .",";
                }
                print "\n";
            }
            
            foreach my $v (@value){
                print $v .",";
            }
            print "\n";
            
            if($value[0] =~ /general/){ $skipa = 1;}
            @name = ();
            @value = ();
            
            
        }
        

