from tkinter import *
from tkinter import scrolledtext
from tkinter import messagebox
import win32com.client
import os
import serial

connected=0
ser=""

Excel = win32com.client.Dispatch("Excel.Application")
path, filename = os.path.split(os.path.abspath(__file__))
wb = Excel.Workbooks.Open(path+u'\setup.xlsx')
sheet = wb.ActiveSheet

window = Tk()
window.title("MA32 Tuner v1")

lbl_select_com= Label(text = "Select COM-port:")
lbl_select_com.pack()

def lbx_on_select_com_port(self,Event=None):
    global connected
    global ser
    if connected==0:
        try:
            ser_name = lbx_com_port_list.get(lbx_com_port_list.curselection())
            ser = serial.Serial(ser_name,115200)        
            connected=1
            cs=lbx_com_port_list.curselection()
            lbx_com_port_list.delete(0,cs[0] -1)
            lbx_com_port_list.delete(1,END)
            stx_from_serial.insert('1.0', ser_name+" opened")
            
        except:            
            connected=0
            messagebox.showerror("Error", "Cannot open COM-port")  

lbx_com_port_list = Listbox(width=40, height=10)
lbx_com_port_list.pack()
lbx_com_port_list.bind('<<ListboxSelect>>', lbx_on_select_com_port)

lbl_res_corr_table=Label(text="RESONANCE CORRECTION TABLE:")
lbl_res_corr_table.pack()

num_of_strings=sheet.Cells(1, 1).CurrentRegion.Rows.Count
frm=[]
ent_from_freq=[]
ent_to_freq=[]
ent_corr_cap=[]
lbl_from_freq=[]
lbl_to_freq=[]
lbl_corr_cap=[]
for i in range(0,num_of_strings):   
    frm.append(Frame(window))
    ent_from_freq.append(Entry(frm[i]))
    ent_to_freq.append(Entry(frm[i]))
    ent_corr_cap.append(Entry(frm[i]))
    lbl_from_freq
    
    try:
        ent_from_freq[i].insert(0,int(sheet.Cells(i+1,1).value))
    except:
        pass
    try:
        ent_to_freq[i].insert(0,int(sheet.Cells(i+1,2).value))
    except:
        pass        
    try:
        ent_corr_cap[i].insert(0,int(sheet.Cells(i+1,3).value))
    except:
        ent_corr_cap[i].insert(0,sheet.Cells(i+1,3).value)
        
    lbl_from_freq.append(Label(frm[i],text="if frequency from"))
    lbl_to_freq.append(Label(frm[i],text="to (Hz)"))
    lbl_corr_cap.append(Label(frm[i],text="change cap to (pF):"))
    lbl_from_freq[i].pack(side=LEFT)    
    ent_from_freq[i].pack(side=LEFT)
    lbl_to_freq[i].pack(side=LEFT)    
    ent_to_freq[i].pack(side=LEFT)
    lbl_corr_cap[i].pack(side=LEFT)    
    ent_corr_cap[i].pack(side=LEFT)
    frm[i].pack()

def save():
    for i in range(0,num_of_strings):
        sheet.Cells(i+1,1).value=ent_from_freq[i].get()
        sheet.Cells(i+1,2).value=ent_to_freq[i].get()
        sheet.Cells(i+1,3).value=ent_corr_cap[i].get()
    wb.save

btn_save=Button(text="SAVE",command=save)
btn_save.pack()    
     
lbl_res_corr_table=Label(text="OUTPUT FROM DEVICE:")
lbl_res_corr_table.pack()

stx_from_serial=scrolledtext.ScrolledText(width = 80,height = 8)
stx_from_serial.pack()


found=False
for i in range(4,64) :
    try :
        port = "COM"+str(i)
        ser = serial.Serial(port)
        ser.close()
        lbx_com_port_list.insert(1, port)
        found = True
    except serial.serialutil.SerialException :
        pass

if not found :    
    messagebox.showerror("Error", "COM-ports not found")

def loop1():
    if connected==1 and ser.in_waiting:        
        received_string=ser.readline()
        if received_string!="":
            stx_from_serial.insert('1.0', received_string)
            try:
                freq=int(received_string[0:4])
            except:
                freq=0

            for i in range(0,num_of_strings):
                try:
                    if freq>=int(ent_from_freq[i].get()) and freq<=int(ent_to_freq[i].get()):                        
                        ent_corr_cap[i].config(background="yellow")
                    else:
                            ent_corr_cap[i].config(background="white")
                except:
                    pass

            
    window.after(100, loop1)
        
loop1()

window.mainloop()

wb.Close()


