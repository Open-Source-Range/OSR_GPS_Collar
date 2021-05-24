### Merge GPS Collar Files from the Version 3 Open-source Range GPS Collar device
"""
This script will input all of the GPSLOG.CSV files in a specified input directory, merge them together, and perform some
basic cleaning and filtering operations. It is best to add the collar ID number or some other unique collar identifier before
the GPSLOG.CSV part of the file name, but the file name must end with GPSLOG.CSV. This script will remove zero values, format
dates and times and convert to local time zone, and do some initial filtering by latitude/longitude box and travel distance/time
to remove spurious points. Output is a formatted CSV file than can be easily converted to point locations in ArcGIS or QGIS.
"""

## Load Libraries
library(stringr)
library(lubridate)
library(dplyr)

### Input parameters
#### NOTE: The script below will add the name of the input GPSLOG files to table for the GPS points and combine all GPSLOG.CSV
####       files into a single output file. It is best to prepend the GPS unit ID or some other unique ID before the GPSLOG.CSV
####       file name to maintain the unique collar identity of the points. e.g., 20027_GPSLOG.CSV.

in.path <- "C:\\Users\\jakal\\OneDrive - University of Idaho\\GPS_Collars\\2020_Collar_Data\\Cody_Morgan_GPS_Data" # Directory of input GPSLOG.CSV files
out.file <- "CodyMorgan2020_Merged.csv" # output CSV file name (will be saved in same directory as input files)
local.time.zone <- "America/Denver" # local time zone. "America/Los_Angeles" is valid code for pacific time zone. See https://www.rdocumentation.org/packages/lubridate/versions/1.7.4/topics/tz for more info.
latbounds <- c(45.5,45.65) # Exclude points outside of a latitude/longitude boundary. Set to c(-90,90) to turn off
lonbounds <- c(-117.1,-116.83) # Exclude points outside of a latitude/longitude boundary. Set to c(-180,180) to turn off
distance.cutoff <- 840 # Remove points where distance between successive points is greater than threshold value (in meters). From Knight processing instructions. Set to very large number to turn off
velocity.cutoff <- 20 # Remove points with calculated velocity (in m/s) above threshold - Very fast cows!! Set to very large number to disable

files <- list.files(in.path,pattern="*GPSLOG.CSV")

merged.data <- data.frame()

for (f in files) {
  data <- read.csv(paste(in.path,f,sep="\\"),header=F,stringsAsFactors = F)
  collar.id <- str_split(f,"[.]")[[1]][1]
  data$collar.id <- collar.id
  names(data)<-c("timeout","locvalid","datevalid","timevalid","year","month","day","hour","min","sec","sats","lat","lon","collar")
  merged.data <- rbind(merged.data,data)
  print(f)
}

## Fix abberant year fields
merged.data$year <- 20

## convert the lat and lon fields to numeric
merged.data$lat <- as.numeric(merged.data$lat)
merged.data$lon <- as.numeric(merged.data$lon)

## Delete blank rows and rows with odd values
merged.data <- merged.data[merged.data$lat>0,]

## Remove records outside of the latitude/longitude range set above
merged.data <- merged.data %>% filter(lat > latbounds[1] & lat < latbounds[2])
merged.data <- merged.data %>% filter(lon > lonbounds[1] & lon < lonbounds[2])


## Create date/time and convert to local time
datetime.string <- str_c(merged.data$year,merged.data$month,merged.data$day,merged.data$hour,merged.data$min,merged.data$sec,sep="-")
utc.datetime <- parse_date_time(datetime.string,"y-m-d-H-M-S",tz="UTC")
merged.data$utc.datetime <- utc.datetime
merged.data$local.datetime <- with_tz(utc.datetime,tzone=local.time.zone)

## remove rows that don't have a date/time
merged.data <- na.omit(merged.data, cols="utc.datetime")

# Calc travel velocity between successive points
filtered.data <- data.frame()
collars <- unique(merged.data$collar)
for (collar.id in collars) {
  collar.point <- merged.data %>% filter(collar == collar.id)
  a <- cbind(as.numeric(collar.point$lon),as.numeric(collar.point$lat))
  b <- cbind(as.numeric(lag(collar.point$lon)),as.numeric(lag(collar.point$lat)))
  b[1,] <- a[1,] # change first value so we don't get NA value
  collar.point$traveldist <- distGeo(a,b)
  collar.point$traveltime <- collar.point$local.datetime - lag(collar.point$local.datetime)
  collar.point$travelvelocity <- collar.point$traveldist/as.numeric(collar.point$traveltime)
  collar.point$traveltime[1] <- 0
  collar.point$travelvelocity[1] <- 0
  filtered.data <- rbind(filtered.data,collar.point)
}

# Remove points greater than the distance cutoff
filtered.data <- filtered.data %>% filter(traveldist < distance.cutoff)
filtered.data <- filtered.data %>% filter(travelvelocity < velocity.cutoff)

# Write out the results
write.csv(filtered.data,paste(in.path,out.file,sep="\\"),col.names=T)

