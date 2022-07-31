# EvolvedCCA
## Why we need Congestion Control algorithm ?

The Network has begined more complexed and huger on the world.  So,the Congestion Control always has been focused at public view of networking domain in recent several years. 

Because the resource is limited, whereas usage in network is bigger than these networking resource provider provides. 

## Conventional methods
For better effectiveness in cc, people adopts three kind of types of methods to slow the problem down.
* First, using fine-grind network info to accurately identify network status. But, It need high precision machine,whereas common machines are difficult for acheiving the standard.

* Second, using AI algorithm to identify networking status, policy-based AI and cc formula-based AI has invoked in recent several years. It also has good effectiveness, but AI is data-driven algorithm.   

* Addationally, mathemestic is a reasonable weapon to solve cc.Traditional methods remain have spaces for sloving cc problem. BBR、Cubic、Westwood and PCC etc both have good mathemestic mechanism. 

## My repo 

I currently has writen down two types of cc algorithm.

### IC_TCP
IC is a compound congestion control algorithm which is made of Cubic and PCC_Vivace, It is inspired by 'Compund TCP' by TanKun. When Cubic is conservation and stable version, you can say that ic is a brilliant version building on the traditional Congestion control 
and have better performance than the others.

It is radical、senstive and its pace is more reasonable.


### Elastic_TCP
Elastic has origined from elastic CCA. because BDP is more important key identity than loss and latency. Addationally, some man-made parameters in formula looks like fake, because it is difficult to enjoy all network status, even network will became complexed and huge in the future.   


