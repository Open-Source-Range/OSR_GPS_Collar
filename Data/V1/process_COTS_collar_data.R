## GPS Collar Data Import and Clean



## Load libraries
library(dplyr)
library(ggplot2)
library(lubridate)
#library(stringr)
library(rgdal)
library(ggmap)
library(geosphere)
library(maptools)

## Set env parameters
path <- "C:\\Users\\jakal\\OneDrive - University of Idaho\\Documents\\GitHub\\COTS_GPS_Collars\\COTS_GPS_Collars\\2018_collar_data\\COTS_Collars"
f <- "001.GPSLOG.CSV"
local.tz <- "America/Denver"
filter.dates <- TRUE
filter.location <- FALSE
date.range <- c("2018-04-27 00:00:00","2018-05-01 23:59:59")
location.range <- c(xmin=-113.453,xmax=-113.433,ymin=42.163,ymax=42.191)
export.points <- FALSE
export.lines <- FALSE
export.dir <- "holding_pasture"
make.map <- FALSE

########################################################
## Functions

## Convert UTC times to local
convert_date_time <- function(df, to.local=TRUE) {
  parse_date_time(test,"%d%m%y",tz="UTC")
}

## Times less than 10am have only seven digits. Add a leading zero.
fix.time.length <- function(time) {
  if (nchar(time)==7){paste("0",time,sep="")} else {time}
}

## Replace dates of 9999 with their antecedant date in the data frame
replace9999 <- function(df, colNum) {
  d <- df[,colNum]
  d.lag <- lag(d,1)
  d.lead <- lead(d,1)
  tmp <- cbind(d,d.lag,d.lead)
  tmp[tmp[,1]==9999,1]<-(tmp[tmp[,1]==9999,2]+tmp[tmp[,1]==9999,3])/2
  df[,colNum]<-tmp
  return(df)
}



########################################################

## Load the data
colNames <- c("HDOP","Lat","Lon","NumSat","UTCDate","UTCTime","FirstFix","LastFix","BestFix")
raw.data <- read.csv(file.path(path,f),col.names=colNames,header=F)


## Count and remove rows with no data
n.missing <- sum(is.na(raw.data$HDOP))
prop.missing <- n.missing/nrow(raw.data)
filtered.data <- raw.data[!is.na(raw.data$HDOP),]

## Replace dates/times with 9999 - occurs when first GPS fix meets the HDOP criteria and unit shuts down before date/time get updated.
## Value for rows with date/time of 9999 will have the date of the preceding observaton and time interpolated from time of preceding and following observations.
## FOR NOW JUST DROP THE 9999 RECORDS (BUT COUNT THEM)
n.9999 <- sum(filtered.data$UTCDate==9999)
prop.9999 <- n.missing/nrow(filtered.data)
filtered.data <- filtered.data %>% filter(UTCDate!=9999)

filtered.data <- cbind(filtered.data,"fixedTime"=sapply(filtered.data$UTCTime,fix.time.length))

## Convert date/time to real date/time field
filtered.data$year <- paste("20",sapply(filtered.data$UTCDate,function(x){substr(x,nchar(x)-1,nchar(x))}),sep="")
filtered.data$month <- sapply(filtered.data$UTCDate,function(x){substr(x,nchar(x)-3,nchar(x)-2)})
filtered.data$day <- sapply(filtered.data$UTCDate,function(x){substr(x,1,nchar(x)-4)})
filtered.data$day <- sapply(filtered.data$day,function(x){if (nchar(x)==1){paste("0",x,sep="")}else{x}}) # Convert all to two-digit day


filtered.data$seconds <- sapply(filtered.data$UTCTime,function(x){substr(x,nchar(x)-3,nchar(x)-2)})
filtered.data$minutes <- sapply(filtered.data$UTCTime,function(x){substr(x,nchar(x)-5,nchar(x)-4)})
filtered.data$hours <- sapply(filtered.data$UTCTime,function(x){substr(x,1,nchar(x)-6)})
filtered.data$hours[filtered.data$hours==''] <- 0 # Trap/fix hours of zero
filtered.data$hours <- sapply(filtered.data$hours,function(x){if (nchar(x)==1){paste("0",x,sep="")}else{x}}) # convert all to two-digit hour


filtered.data$rawDatTime <- paste(paste(filtered.data$day,filtered.data$month,filtered.data$year,sep="/"),paste(filtered.data$hours,filtered.data$minutes,filtered.data$seconds,sep=":"),sep="-")
filtered.data$UTCDateTime <- parse_date_time(filtered.data$rawDatTime,"%d%m%Y-%H%M%S",tz="UTC")
filtered.data$MDTDateTime <- with_tz(filtered.data$UTCDateTime,tzone=local.tz)

## Calculate time difference between subsequent readings
filtered.data$tdiff <- as.numeric(filtered.data$MDTDateTime - lag(filtered.data$MDTDateTime,default=0))
filtered.data$tdiff[1] <- 0 ## Correct the first record.

## Calculate distance between subsequent readings
filtered.data$dist <- mapply(function(Lat1,Lon1,Lat2,Lon2){
                  distGeo(c(Lon1,Lat1),c(Lon2,Lat2))
              },
              Lat1=filtered.data$Lat,Lon1=filtered.data$Lon,Lat2=lag(filtered.data$Lat),Lon2=lag(filtered.data$Lon))
filtered.data$dist[1] <- 0 # Correct the first record

## Calculate velocity between subsequent readings
filtered.data$velocity <- filtered.data$dist/filtered.data$tdiff

## Apply location and date filters
if (filter.dates==TRUE){
  filtered.data <- filtered.data %>% filter(MDTDateTime>=date.range[1] & MDTDateTime<=date.range[2])
}
if (filter.location==TRUE){
  filtered.data <- filtered.data %>% filter(Lon>=location.range['xmin'] & Lon<=location.range['xmax'] & Lat>=location.range['ymin'] & Lat<=location.range['ymax'])
}

## Finally, add an ID field
filtered.data$id <- as.integer(rownames(filtered.data))


## export result as shapefiles for lines and points
if (export.lines) {
  to.lines <- data.frame(Lat2=filtered.data$Lat,Lon2=filtered.data$Lon,Lat1=lag(filtered.data$Lat,default=filtered.data$Lat[1]),Lon1=lag(filtered.data$Lon,default=filtered.data$Lon[1]),id=filtered.data$id-1)
  lines <- mapply(function(Lat1,Lon1,Lat2,Lon2,id){
    Lines(Line(cbind(c(Lon1,Lon2),c(Lat1,Lat2))),ID=id)
  },
  to.lines$Lat1,to.lines$Lon1,to.lines$Lat2,to.lines$Lon2,to.lines$id)
  spatial.lines <- SpatialLinesDataFrame(SpatialLines(lines),data=to.lines,match.ID = FALSE)
  proj4string(spatial.lines)<-CRS("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs")
  writeOGR(spatial.lines,file.path(path,export.dir),paste(substr(f,1,nchar(f)-4),"lines",sep="_"),driver="ESRI Shapefile",overwrite_layer = TRUE)
}

if (export.points) {
  coordinates(filtered.data)<-~Lon+Lat
  proj4string(filtered.data)<-CRS("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs")
  writeOGR(filtered.data,file.path(path,export.dir),substr(f,1,nchar(f)-4),driver="ESRI Shapefile",overwrite_layer = TRUE)
}

## display the results
if (make.map) {
  bbox <- c(left=filtered.data@bbox[1,1],bottom=filtered.data@bbox[2,1],right=filtered.data@bbox[1,2],top=filtered.data@bbox[2,2])
  m <- get_map(location=bbox,maptype="hybrid",zoom=14)
  ggmap(m)+geom_point(data=data.frame(filtered.data), aes(x=Lon,y=Lat,color=MDTDateTime))
}
