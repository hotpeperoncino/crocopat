/**
 * project: BDD width visualization
 * module:  VisualizeFrame.java
 * @version <01.03.2001>
 * @author  <Michael Vogel - (mvogel@informatik.tu-cottbus.de)>
 *
 *
 */

package VisuBDD;

import java.io.*; // Import the necessary classes for file input.
import java.util.*; // Import utilities.

public class FileInput {
  
  /**
   * The method readFile reads the specified input file line by line and parses
   * line contents to a number of type int. The numbers are appended to
   * ta list numberList and returned.
   *
   * @param localList the list containing numberlists already read
   * @param filenameList list containing names of files already read. (in/out)
   * @param filename the name of the current inputfile to be read.
   * @return localList list with added numberlist read from file.
   */

  public static List<List<Double>> readFile(List<List<Double>> localList,
					    List<String> filenameList,
					    String filename) {
  
    List<Double> numberList = new ArrayList<Double>(); // list to store numbers
    String tmpString;               // temporary String holds a
                                    // single line from input file.
    int i = 1;
    double inputNumber = 0.0;       // Current input number read.

    try {   
      FileReader input = new FileReader(filename);
      BufferedReader inputReader = new BufferedReader(input);
      
      System.out.println("\nReading inputfile: " + filename);
      System.out.println("Content: ");
      
      tmpString = inputReader.readLine();       // temp String holds single
                                                // line from input file
      boolean ignoreDescription;                   // Ignoring val descr.

      while (tmpString != null) {               // While not end of file...
        ignoreDescription = false;

        // If dividing list sign was found --> create new list.
        if (tmpString.equals("#")) {
          localList.add(numberList);
          if (i > 1) {
            filenameList.add(new String(filename + "(" + i + ")")); // add filename to list
          } else {
            filenameList.add(filename);             // add filename to list
          } // if
          numberList = new ArrayList<Double>();
          i++;
        } else {
          try {
            // Parse String to double.
            inputNumber = Double.parseDouble(tmpString);
          } catch (NumberFormatException e3) {

            /* Temporary the value description output from the rabbit tool
               will be ignored.

               I need a visualizeation idea to display the descriptions and
               the belonging values.
               I cannot use colors, because of the potentially large number of
               descriptions.

               mvogel 2001-09-05
             */

	    //System.out.println("Ignoring value description.");

           ignoreDescription = true;
          } // catch

          if (!ignoreDescription) {
            numberList.add(Double.valueOf(inputNumber));  // Append Elem to List.
            System.out.println(inputNumber + "; ");
          }
        } // if
        tmpString = inputReader.readLine();
      } // while

      // If last '#' was missing add numberList to localList;
      if (numberList.size() > 0) {
        localList.add(numberList);
          if (i > 1) {
            // add filename to list
            filenameList.add(new String(filename + "(" + i + ")"));
          } else {
            filenameList.add(filename);  // add filename to list
          } // if
        } // if

    } // try

    catch (FileNotFoundException e1) {          // File not Found
      System.out.println("Runtime error in FileInput.readFile(List, List, "
                         + "String).");
      System.out.println(" The specified input file " + filename + " was"
                         + " not found.");
      System.exit(1);
    }
    catch (IOException e2) {                    // IO error
      System.out.println("Runtime error in FileInput.readFile(List, List, "
                         + "String).");
      System.out.println(" An I/O error occured while reading " + filename
                         + " .");
      System.exit(1);
    }
    
    return localList;                          // returns linked List
      
  } // readFile(String filename)

} // class InputFile
