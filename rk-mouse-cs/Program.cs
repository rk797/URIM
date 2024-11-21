class Program
{
    static void Main(string[] args)
    {

        try
        {
            //Fetch based off default paramters
            var mouse = rkMouse.GetMouse();

            int i = 100; 
            Console.WriteLine("Moving mouse...");
            while (i-- > 0)
            {
                mouse.Move(30, 30);
                Thread.Sleep(100);
                mouse.Move(0, 0);
                Thread.Sleep(100);
            }   

        }
        catch (DeviceNotFoundException ex)
        {
            Console.WriteLine(ex.Message);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"An unexpected error occurred: {ex.Message}");
        }

        Console.WriteLine("Press any key to exit.");
        Console.ReadKey();
    }
}
