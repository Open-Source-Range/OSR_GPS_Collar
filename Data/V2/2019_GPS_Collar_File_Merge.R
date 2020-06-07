### Merge GPS Collar Files
library(stringr)
library(lubridate)

in.path <- "C:\\Users\\jakal\\OneDrive - University of Idaho\\GPS_Collars\\CodyMorgan"
out.file <- "CodyMorgan_Merged.csv"

files <- list.files(in.path,pattern="*GPSLOG.CSV")

merged.data <- data.frame()

for (f in files) {
  data <- read.csv(paste(in.path,f,sep="\\"),header=F,stringsAsFactors = F)
  collar.id <- str_split(f,"[.]")[[1]][1]
  data$collar.id <- collar.id
  names(data)<-c("year","month","day","hour","min","sec","status","lat","lon","collar")
  merged.data <- rbind(merged.data,data)
  print(f)
}

## Fix abberant year fields
merged.data$year <- 19

## Delete blank rows
merged.data <- merged.data[merged.data$lat!=0,]

## Create date/time and convert to local time
datetime.string <- str_c(merged.data$year,merged.data$month,merged.data$day,merged.data$hour,merged.data$min,merged.data$sec,sep="-")
utc.datetime <- parse_date_time(datetime.string,"y-m-d-H-M-S",tz="UTC")
merged.data$utc.datetime <- utc.datetime
merged.data$local.datetime <- with_tz(utc.datetime,tzone="America/Denver")

write.csv(merged.data,paste(in.path,out.file,sep="\\"),col.names=T)


# Fix un parsed times in csv
# NAs replaced
