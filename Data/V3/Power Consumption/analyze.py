import pandas as pd
import plotly.express as px
from plotly.offline import plot
import plotly.graph_objects as go
ObservedPeak=70*5 #expected peak wattage
sleep=3*5 #Expected idle wattage
ontime=1
offtime=10
Expected=ObservedPeak*(ontime)/(ontime+offtime)+sleep*(offtime)/(ontime+offtime)
DataPerSecond=1


df = pd.read_csv(r'C:\Users\Mat\Downloads\day1.csv')
df.head()

sums=df.sum()
count=df.max()
mWH=(sums[1]/(count[0]/DataPerSecond))*1000
line1="Expected: "+str(round(Expected,1))+"mWH based on observed behavior in worst case."
line2="Got: "+str(round(mWH,1))+"mWH."





onofflive=[]
maxwattage=0
for i in df.index:
    current=1000*df.iloc[i,1]#not needed, just more readable this way
    if current>sleep*1.1:
        onofflive.append(1)
    else:
        onofflive.append(0)
    if current>maxwattage:
        maxwattage=current

roughontime=0
roughofftime=0
maxontime=0


line3="Peak Wattage(mW): "+str(maxwattage)    



df.insert(2,"State",onofflive,True)

print(df)
fig = px.line(df, x = 'Time (s)', y = 'VP+ Power (W)', title='Instantaneous Wattage Over Time')
fig.add_trace(go.Scatter(
    x=[0,0,0],
    y=[.45,.43,.41],
    mode="text",
    text=[line1,line2,line3],
    textposition="top right"
))

plot(fig)