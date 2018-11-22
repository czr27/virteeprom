# virteeprom
Emulated EEPROM using embedded Flash

There are a lot of cases when you need to use embedded flash that has limitations of rewriting the content.
Actually you must remove all the data from the whole page to get an ability to rewrite the data located 
on the definite address on the page.

The document doc/en.DM00049071.pdf describes one of the mechanism of using embedded flash.
Also there is a patentented method depicted in doc/US7058755.pdf

VirtEEPROM is yet another method of using embedded flash with the wear-leveling technics.
