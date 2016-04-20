/**
 * project: BDD size visualization
 * module:  VisualizeFrame.java
 * @version <01.03.2001>
 * @author  <Michael Vogel     - (mvogel@informatik.tu-cottbus.de)>
 *
 *
 */

package VisuBDD;

import java.awt.Color;          // Import abstract window toolkit.



public class ColorAssignment {

  public static Color getColor(int value) {
    Color resultColor;

    switch (value) {
          case 0:  resultColor = Color.green;
                   break;
          case 1:  resultColor = Color.red;
                   break;
          case 2:  resultColor = Color.blue;
                   break;
          case 3:  resultColor = Color.magenta;
                   break;
          case 4:  resultColor = Color.yellow;
                   break;
          case 5:  resultColor = Color.pink;
                   break;
          default: resultColor = Color.black;
        } // switch

    return resultColor;
  } // getColor
  
} // class
