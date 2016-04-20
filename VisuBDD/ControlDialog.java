/**
 * project: BDD width visualization
 * module:  VisualizeFrame.java
 * @version <01.03.2001>
 * @author  <Michael Vogel - (mvogel@informatik.tu-cottbus.de)>
 */

package VisuBDD;

import java.awt.*;          // Import abstract window toolkit.
import java.awt.event.*;    // Import event components for awt.


public class ControlDialog extends Dialog {

  private VisualizeFrame mainFrame;
  private Panel controlPanel, buttonPanel, buttonPanel2, lineDistancePanel;
  private TextField maxRange;
  private CheckboxGroup group;
  private Checkbox val1, val2, val3, xorBox;

  /**
   * The constructor assigns the owner argument to class variable owner
   * and adds a WindowListener for closing events to this window.
   *
   * @param owner reference to owner frame
   */
  public ControlDialog(Frame mainWindow) {
    super(mainWindow); // call constructor of superclass.

    Label maxValLabel, distanceLabel;
    Button startButton;

    this.mainFrame = (VisualizeFrame) mainWindow;

    maxValLabel = new Label("Maximum BDD Width");
    maxRange = new TextField(10);
    group = new CheckboxGroup();

    distanceLabel = new Label("Vertical Stretching");
    val1 = new Checkbox("1", false, group);
    val2 = new Checkbox("2", true, group);
    val3 = new Checkbox("3", false, group);


    xorBox = new Checkbox("Combine Colors", false);

    startButton = new Button("Repaint");
    startButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent event) {
        //System.out.println("Button actionPerformed: " + maxRange.getText());
        try {
          mainFrame.setMaxBddWidth(Double.parseDouble(maxRange.getText()));
        } catch (NumberFormatException e1) {
          System.out.println("\nYou have to type only double values into the"
                             + " textfield.");
        } // catch

        mainFrame.setDistance(checkDistanceValue());
        mainFrame.setXorMode(xorBox.getState());
        mainFrame.repaint();
      } // actionPerformed
    } );

    controlPanel = new Panel(new FlowLayout());
    controlPanel.add(maxValLabel);
    controlPanel.add(maxRange);

    buttonPanel = new Panel(new FlowLayout());
    buttonPanel.add(xorBox);

    buttonPanel2 = new Panel(new FlowLayout());
    buttonPanel2.add(startButton);

    lineDistancePanel = new Panel(new FlowLayout());
    lineDistancePanel.add(distanceLabel);
    lineDistancePanel.add(val1);
    lineDistancePanel.add(val2);
    lineDistancePanel.add(val3);

    setLayout(new GridLayout(0,1));
    add(controlPanel);
    add(lineDistancePanel);
    add(buttonPanel);
    add(buttonPanel2);

    // Initialization of variables.
    setResizable(false);
    //setBackground(Color.white);
    setTitle("Preferences"); // Set window title.
    setLocation(50, 100);// Set initial window position.

    pack();

    // Add WindowListener for closing event.
    addWindowListener(
      new WindowAdapter() {
        public void windowClosing(WindowEvent evt) {
          dispose(); // close
        } // windowClosing(WindowEvent evt)
      } // WindowAdapter
    ); // addWindowListener(new WindowAdapter)

  } // constructor

  private int checkDistanceValue() {
    int result = 0;

    if (val1.getState()) {
      result = 1;
    } else if (val2.getState()) {
      result = 2;
    } else if (val3.getState()) {
      result = 3;
    } else {
      System.out.println("Runtime error in ControlDialog.checkDistance"
                         + "Value()");
      System.out.println(" There is a selection error in the checkbox"
                         + " group.");
    } // if
    return result;
  } // checkDistanceValue

  public void setMaxBDDWidth(double value) {
    maxRange.setText(String.valueOf(value));
  } // setMaxBDDWidth
  
} // class
