/**
 * project: BDD width visualization
 * module:  VisualizeFrame.java
 * @version <01.03.2001>
 * @author  <Michael Vogel - (mvogel@informatik.tu-cottbus.de)>
 *
 *
 */

package VisuBDD;

import java.awt.*;          // Import abstract window toolkit.
import java.awt.event.*;    // Import event components for awt.


public class VisualizeFrame extends Frame {

  private Frame myFrame;                // Pointer to this frame;
  private ScrollPane scrPane;
  private MyCanvas canvas;               // Canvas for graphics.
  private java.util.List<java.util.List<Double>> visualizeList;  // contains value lists for display
  private int lineDistance = 2;
  private double maxBddWidth;
  private int[] fileSequence;
  private boolean xorMode = false;
  double[][] maxVals;

  /*
   * Class represents the canvas object, the line diagram will be draw on.
   * The class method paint, draws the diagram.
   */

  private class MyCanvas extends Canvas {
    
      /**
     *
     */

    private int maxBDDdepth(java.util.List<java.util.List<Double>> localList) {
      int maxLength = 0;

      for (java.util.List<Double> l : localList) {
	if (l.size() > maxLength) {
          maxLength = l.size();
        }
      }

      return maxLength;
    }

    /*
     * Draws the line diagram
     *
     * @param area the graphics area for drawing.
     */

    public void paint(Graphics area) {
      int xStart, xEnd, // minimal size of window.
          lineWidth,hSize, vSize;
      double tmpVal, tmpVal2;

      java.util.List<Double> numberList;

      if (xorMode) {
        area.setXORMode(Color.white);
      } else {
        area.setPaintMode();
      }

      for (int i = 0; i < visualizeList.size(); i++) {
         maxVals[i][0] = max(visualizeList.get(i));
         maxVals[i][1] = i;
      }

      // BubbleSort (descending sort)
      for (int i = 0; i < maxVals.length - 1; i++) {
        for (int j = i + 1; j < maxVals.length; j++) {
           if (maxVals[i][0] < maxVals[j][0]) {
             tmpVal = maxVals[i][0];
             tmpVal2 = maxVals[i][1];
             maxVals[i][0] = maxVals[j][0];
             maxVals[i][1] = maxVals[j][1];
             maxVals[j][0] = tmpVal;
             maxVals[j][1] = tmpVal2;
           }
        }
      }

      fileSequence = new int[maxVals.length];
      for (int i = 0; i < maxVals.length; i++) {
        fileSequence[i] = (int) maxVals[i][1];
      }

      //maxVal = maxVals[0][0]; //((float) this.max());

      area.drawString("Maximum BDD Width: " + maxBddWidth, 0, 12);

/*
      lineDistance = (myFrame.getSize().height - 60)
                     / this.maxBDDdepth(visualizeList);

      if (lineDistance > 3) {
        lineDistance = 3;
      } else if (lineDistance == 0) {
        lineDistance = 1;
      }
*/

      hSize = myFrame.getSize().width - 35;  // save window sizes.
      vSize = maxBDDdepth(visualizeList) * lineDistance + 30;
      this.setSize(hSize, vSize);

      //System.out.println("hSize = " + hSize + " | vSize = " + vSize);

      for (int i = 0; (i < visualizeList.size()); i++) {
        area.setColor(ColorAssignment.getColor(i));
        numberList = visualizeList.get((int) maxVals[i][1]);

        for (int j = 0; (j < numberList.size()) && (j < hSize); j++) {
          lineWidth =  (int) (( (hSize - 20) / maxBddWidth)
               * numberList.get(j).doubleValue());
          xStart = (hSize - lineWidth) / 2;
          xEnd = xStart + lineWidth;

          for (int k = 0; k < lineDistance; k++) {
            area.drawLine(xStart,(lineDistance * j + 20 + k),
                          xEnd, (lineDistance * j + 20 + k));
          }
        }
      }
    } // paint


    public Dimension getPreferedSize() {
      return new Dimension(myFrame.getSize().width - 50,
                           myFrame.getSize().height - 50);
    }

  } // class My_Canvas
  

  /**
   * The constructor assigns the owner argument to class variable owner
   * and adds a WindowListener for closing events to this window.
   *
   * @param owner reference to owner frame
   */
  
  public VisualizeFrame(java.util.List<java.util.List<Double>> visualizeList) {
    double tmpVal, tmpVal2;

    // Initialization of variables.
    this.myFrame = this;
    this.setTitle("Visualization of BDD Shape"); // Set window title.
    this.setLocation(295, 100);            // Set initial window position.
    this.setSize(600,500);             // Set windows size.

    this.visualizeList = visualizeList;
    this.maxVals = new double[visualizeList.size()][2];

    for (int i = 0; i < visualizeList.size(); i++) {
      this.maxVals[i][0] = max(this.visualizeList.get(i));
      this.maxVals[i][1] = i;
    }

    // BubbleSort (descending sort)
    for (int i = 0; i < maxVals.length - 1; i++) {
      for (int j = i + 1; j < maxVals.length; j++) {
        if (maxVals[i][0] < maxVals[j][0]) {
          tmpVal = maxVals[i][0];
          tmpVal2 = maxVals[i][1];
          maxVals[i][0] = maxVals[j][0];
          maxVals[i][1] = maxVals[j][1];
          maxVals[j][0] = tmpVal;
          maxVals[j][1] = tmpVal2;
        }
      }
    }

    this.fileSequence = new int[maxVals.length];
    for (int i = 0; i < maxVals.length; i++) {
      fileSequence[i] = (int) maxVals[i][1];
    }


    // Add WindowListener for closing event.
    addWindowListener(
      new WindowAdapter() {
        public void windowClosing(WindowEvent evt) {
          dispose(); // close
          System.exit(0);
        }
      }
    );

    prepareWindow();  // Add windows components.
    canvas.repaint(); // Initial draw.


  } // constructor
  

  /**
   *
   */

  public double max(java.util.List<Double> numberList) {
    double maxVal = 0.0;

    for (Double d : numberList) {
      if (d.doubleValue() > maxVal) {
        maxVal = d.doubleValue();
      }
    }

    return maxVal;
  }

/**
 *
 */

  public void setMaxBddWidth(double value) {
    this.maxBddWidth = value;
  }

  public void setDistance(int value) {
    this.lineDistance = value;
  }

  public void setXorMode(boolean xorMode) {
    this.xorMode = xorMode;
  }

  public int[] getFileSequence() {
//    System.out.println("getFileSequence length=" + this.fileSequence.length);
    return this.fileSequence;
  }

  /**
   *
   */

  public void repaint() {
    this.canvas.repaint();
  }

  /**
   * Defines the appearance of this window and adds the canvas containing
   * the line diagramm.
   */
  
  private void prepareWindow() {
    this.canvas = new MyCanvas(); // initialize canvas.
    this.canvas.setBackground(Color.white);
    this.scrPane = new ScrollPane();

    this.scrPane.add(this.canvas);

    // Schrittweite der Scrollbars auf 25 Pixel je Mausklick einstellen. 
//    this.scrPane.getVerticalScrollBar().setUnitIncrement(25);


    // Add the graphics canvas to the frames content pane.
    this.add(scrPane);

  }

  public Dimension getPreferedSize(){
    return new Dimension(600, 400);
  }


} // class
