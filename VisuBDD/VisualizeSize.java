/**
 * project: BDD width visualization
 * module:  VisualizeSize.java
 * @version <01.03.2001>
 * @author  <Michael Vogel - (mvogel@informatik.tu-cottbus.de)>
 *
 *
 */

package VisuBDD;

import java.util.*; // Import utilities.
import VisuBDD.VisualizeFrame;        // import class for visualization
import VisuBDD.FileInput;             // import class for file input
import VisuBDD.FilenameDialog;
import VisuBDD.ControlDialog;

public class VisualizeSize {

  /**
   *
   */

  public static double maxVal(java.util.List<java.util.List<Double>> localList) {
    double maxVal = 0.0;

    for (java.util.List<Double> l : localList) {
      for (Double d : l) {
        if (d.doubleValue() > maxVal) {
          maxVal = d.doubleValue();
        } // if
      } // for
    } // for

    return maxVal;
  } // max



  /**
   * main opens the main window of this application.
   * 
   * @param args[0] input filename
   */
  
  public static void main(String[] args) {
    VisualizeFrame mainWindow;
    FilenameDialog filenameWindow;
    ControlDialog controlWindow;
    java.util.List<java.util.List<Double>> visualizeList;
    java.util.List<String> filenames;

    if (args.length > 0) {
      visualizeList = new ArrayList<java.util.List<Double>>();
      filenames = new ArrayList<String>();

      // read inputFiles
      for (int i = 0; i < args.length; i++) {
        visualizeList = FileInput.readFile(visualizeList, filenames, args[i]);
      } // for

      // Call visualization frame with visualizeList and filename.
      mainWindow = new VisualizeFrame(visualizeList);
      mainWindow.setMaxBddWidth(maxVal(visualizeList));
      mainWindow.setDistance(2); // initial val equal to control val.

      controlWindow = new ControlDialog(mainWindow);
      controlWindow.setMaxBDDWidth(maxVal(visualizeList));

      filenameWindow = new FilenameDialog(filenames, mainWindow);
      mainWindow.setVisible(true);
      filenameWindow.setVisible(true);
      controlWindow.setVisible(true);

    } else {
      System.out.println("BDD width visualizer");
      System.out.println("\nusage: java VisualizeSize inputfilename1 "
                         + "[inputfilename2] ...");
      System.exit(1);
    } // if
  } // main

} // class TimeDisplays
