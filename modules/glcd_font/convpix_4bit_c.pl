# $Id: convpix_c.pl 43 2008-09-30 23:30:38Z tk $
#!/usr/bin/perl -w
#
# original is convpix_c.pl by Thorsten Klose (initial version 8/95, C output version 9/2008)
# this version is used to convert 8bit grayscale graphic to asm-data for 4bit depth pixel graphic display
# notes: there's 2 pixels by byte, first pixel is in MSB nibble, second in LSB
#
# Howto:
# a) Choose a font, 'bitmap fonts' are best suitted here, you can find some on well known fonts library like DaFont website.
# b) Download and install Bitmap Font Builder from www.LMNOpc.com, it's free, import the font of your choice
#    put the texture size to 'AUTO-0', the Font Smoothing to enabled and save as bmp.
# c) Download and install Gimp2, GNU, open the bitmap you created before, Image>Canvas Size... and divide height size by 2.
#    Check and correct the color depth, Image>Mode<Grayscale, now posterize: Colors>Posterize and set levels to 16.
#    Export as xpm file.
# d) Copy the xpm in the same folder as conv8bitGraypix_c4bit.pl, cmd: perl conv8bitGraypix_c4bit.pl yourfont.xpm
# => datas are in the inc file of the same name.

use Getopt::Long;

$debug = 0;
$icons_per_line = 0;
$height = 8;

$cmd_args = join(" ", @ARGV);

GetOptions (
   "debug" => \$debug,
   "icons_per_line=s" => \$icons_per_line,
   "height=s" => \$height,
   );

if( scalar(@ARGV) != 1 )
{
   die "SYNTAX:   convpix.pl <xpm-file> [<-debug>]\n" .
       "EXAMPLE:  convpix.pl font.xpm\n";
}

$input_file=$ARGV[0];

if( $height % 8 )
{
   die "ERROR: height must be multiple of 8!\n";
}

$output_file=sprintf("%s.inc",substr($input_file, 0, length($input_file)-4));

open(IN,"<$input_file");

my @graphic = ();
my $state = "WAIT_RESOLUTION";
my $y = 0;
my $x_max;
my $y_max;
my $c_num;
my @colors_cod = ();
my @colors = ();
my $colors_cod_num;

while( <IN> )
{
   $line=$_;
   chomp($line);

   print "[${state}] ${line}\n";

   if( $state eq "READ_HEADER" )
   {   
      if( $line ne "/* XPM */" )
      {
         die "ERROR: Expecting .xpm Format!\n";
      }
      $state = "WAIT_RESOLUTION";
   }
   elsif( $state eq "WAIT_RESOLUTION" )
   {
      if( /^[\s]*\"(\d+) *(\d+) *(\d+) *(\d+)\",/ )
      {
         $x_max = $1;
         $y_max = $2;
         $c_num = $3;
         $colors_cod_num = $4;
#         if( $y_max % 8 )
#         {
#            die "ERROR: Y-Size must be dividable by 8!\n";
#         }
#
#         if( $x_max % 8 )
#         {
#            die "ERROR: X-Size must be dividable by 8!\n";
#         }
        
         if( $colors_cod_num >1 )
         {
            die "ERROR: Too much colors in the table\n";
            die "ERROR: Please apply a 16 level posterize before convert!\n";
         }
         print "Image Size: $x_max * $y_max and $c_num colors\n";

         $state = "WAIT_COLOR";
      }
   }
   elsif( $state eq "WAIT_COLOR" )
   {
      if( /^[\s]*\".*\",/ )
      {
        my $search_string = "g #";
        if($line =~ m/\Q$search_string/){
          push(@colors_cod,substr($line, 1, 1));
          push(@colors,substr($line, 6, 1));
          if(scalar @colors_cod ne $c_num){
            $state = "WAIT_COLOR";
          }else{
            $state = "READ_PIXELS";
          }
        }else{
          die "ERROR: Must be grayscale!\n";
        }

      }
   }
   elsif( $state eq "WAIT_COLOR_1" )
   {
      if( /^[\s]*\".*\",/ )
      {
         $state = "READ_PIXELS";
      }
   }   
   elsif( $state eq "READ_PIXELS" )
   {
      if( /^[\s]*\/\*/ )
      {
         # do nothing
      }
      else
      {   
         if( substr($line, 0, 1) ne "\"" || substr($line, $x_max+1, 1) ne "\"" )
         {
            die "ERROR: Wrong Pixel Size - expecting x = $x_max!\n";
         } 

         for($i=1;$i<=$x_max;++$i)
         {
            push(@graphic,substr($line, $i, 1));
         }

         if( ++$y == $y_max )
         {
            $state = "READ_BOTTOM";
         }
      }   
   }      
   elsif( $state eq "READ_BOTTOM" )
   {
      if( $line ne "};" )
      {
         die "ERROR: Wrong Pixel Size - expecting y = $x_max!\n";
      }
   }   
   else
   {   
      die "FATAL: Unknown State: ${state}\n";
   }
}   
close(IN);

my $expected_size = $x_max * $y_max;
if( scalar(@graphic) != $expected_size )
{
    die sprintf("FATAL: Wrong Graphic Size: %d (has to be $expected_size)!\n",scalar(@graphic));
}

my $character_offset = 0;
if( $icons_per_line )
{
   $character_offset = $x_max / $icons_per_line;
   printf "Calculated Character Offset: %d\n", $character_offset;
   printf "Character Height:            %d\n", $height;

   if( $y_max % $height )
   {
      die "ERROR: pic height doesn't comply with character height\n";
   }

   my @new_graphic = ();

   my $x=0;
   my $new_x_max = ($x_max * $y_max) / 8;
   my $i;
   my $j;
   my $k;
   my $l;
   my $m;
   for($i=0; $i<$y_max/$height; ++$i)
   {
      for($j=0; $j<$icons_per_line; ++$j)
      {
	 for($l=0; $l<$height/8; ++$l)
	 {
            for($k=0; $k<$character_offset; ++$k)
            {
	       for($m=0; $m<8; ++$m)
	       {
		  my $ix = ($j*$character_offset + $k) + (($i*$height + $l*8 + $m)*$x_max);
		  my $new_ix = $x + ($m*$new_x_max);
		  $new_graphic[$new_ix] = $graphic[$ix];
#		  printf "%2d %2d %2d %2d %2d -> %3d/%3d -> %s [%3d/%3d]\n", $i, $j, $k, $l, $m, $ix % $x_max, $ix / $x_max, $graphic[$ix], $new_ix % $new_x_max, $new_ix / $new_x_max;
	       }
	       ++$x;
	    }
	 }
      }
   }
   @graphic = @new_graphic;
   $x_max = $new_x_max;
   $y_max = 8;
}

open(OUT,">$output_file");

printf OUT "// converted with 'convpix %s'\n\n", $cmd_args;

if( 0 ) # for debugging
{
   for($h=0; $h<$y_max; ++$h)
   {
      for($i=0; $i<$x_max; ++$i)
      {
         print OUT $graphic[$h*$x_max + $i];
      }   
      print OUT "\n";
   }
   close(OUT);
   exit;
}

my @lines = ();
for($h=0; $h<$y_max; ++$h)
{
  my @bytes = ();
  my $b = 0;
  for($i=0; $i<$x_max; ++$i) {

    foreach my $j (0 .. $#colors){
      if( $graphic[($h*$x_max) + $i] eq $colors_cod[$j] ) {
        my $string = sprintf("0x0%s", $colors[$j]);
        my $val = hex($string);
        if(($i & 1) == 0){
          $b = ($val << 4);
        }else{
          $b |= ($val & 0x0f);
          push @bytes, sprintf("0x%02x", $b);
        }
      }
    }
  }

  my @line = ();
  foreach $b (@bytes) {
    push @line, $b;
    if( scalar(@line) == 16 ) {
      push @lines, join(",", @line);
      @line = ();
    }
  }

  if( scalar(@line) ) {
    push @lines, join(",", @line);
  }
}

printf OUT join(",\n", @lines) . "\n";
close(OUT);
print "Ok.\n";

