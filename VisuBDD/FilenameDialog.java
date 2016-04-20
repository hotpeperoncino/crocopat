/**
 * project: BDD size visualization
 * module:  VisualizeFrame.java
 * @version <01.03.2001>
 * @author  <Michael Vogel     - (mvogel@informatik.tu-cottbus.de)>
 *
 *
 */

package VisuBDD;

import java.awt.*;          // Import abstract window toolkit.
import java.awt.event.*;    // Import event components for awt.


public class FilenameDialog extends Dialog {

  /**
   * The constructor assigns the owner argument to class variable owner
   * and adds a WindowListener for closing events to this window.
   *
   * @param owner reference to owner frame
   */
  
  public FilenameDialog(java.util.List<String> filenames, Frame mainWindow) {
    super(mainWindow); // call constructor uof superclass.

    Label tmpLabel;
    int[] fileSequence;

    fileSequence = ((VisualizeFrame) mainWindow).getFileSequence();
    // Initialization of variables.
    setResizable(false);
    setBackground(Color.white);
    setTitle("Legend"); // Set window title.
    setLocation(50, 260);// Set initial window position.
    setLayout(new GridLayout(0, 1)); // n rows, 1 column

    for (int i = 0; i < filenames.size(); i++) {
      tmpLabel = new Label();
      tmpLabel.setForeground(ColorAssignment.getColor(i));
      tmpLabel.setText(filenames.get(fileSequence[i]));
      add(tmpLabel);
    } // for

    pack();

    // Add WindowListener for closing event.
    addWindowListener(
      new WindowAdapter() {
        public void windowClosing(WindowEvent evt) {
          dispose(); // close
          //System.exit(0);
        } // windowClosing(WindowEvent evt)
      } // WindowAdapter
    ); // addWindowListener(new WindowAdapter)

  } // constructor
} // class
