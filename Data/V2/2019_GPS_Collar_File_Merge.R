# Merge GPS Collar Files
library(stringr)
library(lubridate)
library(ggplot2)
library(geosphere)

in.path <- "C:\\Users\\jakal\\OneDrive - University of Idaho\\GPS_Collars\\2020_Collar_Data\\Cody_Morgan_GPS_Data"
out.file <- "CodyMorgan2020_Merged.csv"
latbounds <- c(44.3,44.6)
lonbounds <- c(-113.9,-113.7)
datebounds <- c("2020-04-20","2020-07-11")
distance.cutoff <- 840 # meters. From Knight processing instructions


# Generate list of files
files <- list.files(in.path,pattern="*GPSLOG.CSV")

# setup empty data frame to hold results
merged.data <- data.frame()

# load and merge data files
for (f in files) {
  data <- read.csv(paste(in.path,f,sep="\\"),header=F,stringsAsFactors = F)
  collar.id <- str_split(f,"[.]")[[1]][1]
  data$collar.id <- collar.id
  names(data)<-c("year","month","day","hour","min","sec","status","lat","lon","collar")
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
merged.data$local.datetime <- with_tz(utc.datetime,tzone="America/Denver")

## remove rows that don't have a date/time
merged.data <- na.omit(merged.data, cols="utc.datetime")

## remove points outside of date range
merged.data <- merged.data %>% filter(local.datetime >= datebounds[1] & local.datetime <= datebounds[2])

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
#filtered.data <- filtered.data %>% filter(travelvelocity < velocity.cutoff)

# Write out the results
write.csv(filtered.data,paste(in.path,out.file,sep="\\"),col.names=T)

## Summary stats
# collar points per day
filtered.data$yday <- yday(filtered.data$local.datetime)
collar.sums <- filtered.data %>% group_by(collar) %>% summarise(firstday=min(yday),lastday=max(yday),npoints=n())
collar.sums$duration <- collar.sums$lastday - collar.sums$firstday
collar.sums$infodens <- collar.sums$npoints / collar.sums$duration
ggplot(data=collar.sums)+geom_histogram(aes(x=infodens))
ggplot(data=collar.sums)+geom_bar(aes(x=collar,y=infodens))



