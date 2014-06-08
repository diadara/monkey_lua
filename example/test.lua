config = mk.config
r = mk.request

mk.print("Hello World")

i = 1
for k, v in pairs(r)
do
   mk.print(i .. " " .. k .. " " .. tostring(v))
   i = i + 1
end
   
mk.print("last line !")

i = 1
for k,v in pairs(config) do
   mk.print(i .. " " .. k .. " " .. tostring(v))
   i = i + 1
end
